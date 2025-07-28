#include "RTMPPuller.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "librtmp/log.h"
#include "librtmp/rtmp_sys.h"
#ifdef __cplusplus
}
#endif

#include <stdio.h>
#define DEF_TIMEOUT 30                    /* seconds */
#define DEF_BUFTIME (10 * 60 * 60 * 1000) /* 10 hours default */
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

#include "../Logger/Logger.h"
#include "../Util/TimeHelper.h"
#include "../Util/timeutil.h"
#include "librtmp/rtmp_sys.h"

using namespace TimeHelper;
using namespace Time;

namespace TransProtocol {

RTMPPuller::RTMPPuller(std::string url) : url_(url) {
  rtmpProtocol_ = std::make_shared<RTMPProtocol>(url_, RTMP_BASE_TYPE_PLAY);
  if (!rtmpProtocol_) {
    LOG_ERROR("Failed to create IRTMPProtocol instance");
  }
}

RTMPPuller::~RTMPPuller() { stop(); }

void RTMPPuller::parseScriptTag(RTMPPacket &packet) {
  LOG_INFO("begin parse info %d", packet.m_nBodySize);
  AMFObject obj;
  AVal val;
  AMFObjectProperty *property;
  AMFObject subObject;
  if (AMF_Decode(&obj, packet.m_body, packet.m_nBodySize, FALSE) < 0) {
    LOG_INFO("%s, error decoding invoke packet", __FUNCTION__);
  }
  AMF_Dump(&obj);
  LOG_INFO(" amf obj %d", obj.o_num);
  for (int n = 0; n < obj.o_num; n++) {
    property = AMF_GetProp(&obj, NULL, n);
    if (property != NULL) {
      if (property->p_type == AMF_OBJECT) {
        AMFProp_GetObject(property, &subObject);
        for (int m = 0; m < subObject.o_num; m++) {
          property = AMF_GetProp(&subObject, NULL, m);
          LOG_INFO("val = %s", property->p_name.av_val);
          if (property != NULL) {
            if (property->p_type == AMF_OBJECT) {

            } else if (property->p_type == AMF_BOOLEAN) {
              int bVal = AMFProp_GetBoolean(property);
              if (strncasecmp("stereo", property->p_name.av_val,
                              property->p_name.av_len) == 0) {
                audio_channel = bVal > 0 ? 2 : 1;
                LOG_INFO("parse channel %d", audio_channel);
              }
            } else if (property->p_type == AMF_NUMBER) {
              double dVal = AMFProp_GetNumber(property);
              if (strncasecmp("width", property->p_name.av_val,
                              property->p_name.av_len) == 0) {
                video_width = (int)dVal;
                LOG_INFO("parse widht %d", video_width);
              } else if (strcasecmp("height", property->p_name.av_val) == 0) {
                video_height = (int)dVal;
                LOG_INFO("parse Height %d", video_height);
              } else if (strcasecmp("framerate", property->p_name.av_val) ==
                         0) {
                video_frame_rate = (int)dVal;
                LOG_INFO("parse frame_rate %d", video_frame_rate);
                if (video_frame_rate > 0) {
                  video_frame_duration_ = 1000 / video_frame_rate;
                }
              } else if (strcasecmp("videocodecid", property->p_name.av_val) ==
                         0) {
                video_codec_id = (int)dVal;
                LOG_INFO("parse video_codec_id %d", video_codec_id);
              } else if (strcasecmp("audiosamplerate",
                                    property->p_name.av_val) == 0) {
                audio_sample_rate = (int)dVal;
                audio_sample_rate = 32000;
                LOG_INFO("parse audiosamplerate %d", audio_sample_rate);
              } else if (strcasecmp("audiodatarate", property->p_name.av_val) ==
                         0) {
                audio_bit_rate = (int)dVal;
                LOG_INFO("parse audiodatarate %d", audio_bit_rate);
              } else if (strcasecmp("audiosamplesize",
                                    property->p_name.av_val) == 0) {
                audio_sample_size = (int)dVal;
                LOG_INFO("parse audiosamplesize %d", audio_sample_size);
              } else if (strcasecmp("audiocodecid", property->p_name.av_val) ==
                         0) {
                audio_codec_id = (int)dVal;
                LOG_INFO("parse audiocodecid %d", audio_codec_id);
              } else if (strcasecmp("filesize", property->p_name.av_val) == 0) {
                file_size = (int)dVal;
                LOG_INFO("parse filesize %d", file_size);
              }
            } else if (property->p_type == AMF_STRING) {
              AMFProp_GetString(property, &val);
            }
          }
        }
      } else {
        AMFProp_GetString(property, &val);

        LOG_INFO("val = %s", val.av_val);
      }
    }
  }
}

void RTMPPuller::start() {
  rtmpProtocol_->init();
  worker_ = std::thread(std::bind(&RTMPPuller::readPacketThread, this));
  worker_.detach();
}

void RTMPPuller::stop() {
  request_exit_thread_ = true;
  sync_thread_.wait(std::unique_lock<std::mutex>(mutex_));
}

void RTMPPuller::readPacketThread() {
  AVPlayTime *play_time = AVPlayTime::GetInstance();

  RTMPPacket packet = {0};
  int64_t cur_time = TimesUtil::GetTimeMillisecond();
  int64_t pre_time = cur_time;
  static FILE *rtmp_dump_h264 = NULL;

  uint8_t *nalu_buf = (uint8_t *)malloc(1 * 1024 * 1024);
  int total_slice_size = 0;
  int total_slice_offset = 0;
  try {
    while (!request_exit_thread_) {
      // 短线重连
      if (!rtmpProtocol_->isConnected()) {
        LOG_INFO("短线重连 re connect");
        if (!rtmpProtocol_->connect()) // 重连失败
        {
          LOG_INFO("短线重连 reConnect fail %s", url_.c_str());
          msleep(10);
          continue;
        }
      }
      cur_time = TimesUtil::GetTimeMillisecond();
      int64_t t = cur_time - pre_time;
      pre_time = cur_time;
      if (!rtmpProtocol_->readPacket(&packet)) {
        LOG_INFO("RTMP_ReadPacket failed");
        continue;
      }
      int64_t diff = TimesUtil::GetTimeMillisecond() - cur_time;
      if (rtmpProtocol_->isReadCompleted(&packet)) // 检测是不是整个包组好了
      {
        diff = TimesUtil::GetTimeMillisecond() - cur_time;
        if (diff > 10) {
          bool keyframe = false;
          if (packet.m_packetType == RTMP_PACKET_TYPE_VIDEO) {
            keyframe = 0x17 == packet.m_body[0] ? true : false;
          }
        }
        uint8_t nalu_header_4bytes[4] = {0x00, 0x00, 0x00, 0x01};
        uint8_t nalu_header_3bytes[3] = {0x00, 0x00, 0x01};
        // Process packet, eg: set chunk size, set bw, ...
        //            RTMP_ClientPacket(m_pRtmp, &packet);
        if (!packet.m_nBodySize)
          continue;
        std::cout << "RTMPPacket_m_packetType: "
                  << static_cast<int>(packet.m_packetType) << std::endl;
        if (packet.m_packetType == RTMP_PACKET_TYPE_VIDEO) {
          // 解析完数据再发送给解码器
          // 判断起始字节, 检测是不是spec config, 还原出sps pps等
          // 重组帧
          bool keyframe = 0x17 == packet.m_body[0] ? true : false;
          bool sequence = 0x00 == packet.m_body[1];
          // SPS/PPS sequence
          if (keyframe && sequence) {
            LOG_INFO("%s:%s:t:%u", play_time->getRtmpTag(),
                     play_time->getAvcHeaderTag(), play_time->getCurrenTime());
            is_got_video_sequence_ = true;
            uint32_t offset = 10;
            uint32_t sps_num = packet.m_body[offset++] & 0x1f;
            if (sps_num > 0) {
              sps_vector_.clear(); // 先清空原来的缓存
            }
            for (int i = 0; i < sps_num; i++) {
              uint8_t ch0 = packet.m_body[offset];
              uint8_t ch1 = packet.m_body[offset + 1];
              uint32_t sps_len = ((ch0 << 8) | ch1);
              offset += 2;
              // Write sps data
              std::string sps;
              sps.append(nalu_header_4bytes,
                         nalu_header_4bytes + 4); // 存储 start code
              sps.append(packet.m_body + offset,
                         packet.m_body + offset + sps_len);
              sps_vector_.push_back(sps);
              offset += sps_len;
            }
            uint32_t pps_num = packet.m_body[offset++] & 0x1f;
            if (pps_num > 0) {
              pps_vector_.clear(); // 先清空原来的缓存
            }
            for (int i = 0; i < pps_num; i++) {
              uint8_t ch0 = packet.m_body[offset];
              uint8_t ch1 = packet.m_body[offset + 1];
              uint32_t pps_len = ((ch0 << 8) | ch1);
              offset += 2;
              // Write pps data
              std::string pps;
              pps.append(nalu_header_4bytes,
                         nalu_header_4bytes + 4); // 存储 start code
              pps.append(packet.m_body + offset,
                         packet.m_body + offset + pps_len);
              pps_vector_.push_back(pps);
              offset += pps_len;
            }

            // 封装成avpacket
            AVPacket sps_pkt = {0};
            sps_pkt.size = sps_vector_[0].size();
            sps_pkt.data = (uint8_t *)av_malloc(sps_pkt.size);
            if (av_packet_from_data(&sps_pkt, sps_pkt.data, sps_pkt.size) ==
                0) {
              memcpy(sps_pkt.data, (uint8_t *)sps_vector_[0].c_str(),
                     sps_vector_[0].size());
              if (!rtmp_dump_h264) {
                rtmp_dump_h264 = fopen("rtmp.h264", "wb+");
              }
              fwrite(sps_pkt.data, sps_pkt.size, 1, rtmp_dump_h264);
              fflush(rtmp_dump_h264);
              if (video_packet_callable_object_)
                video_packet_callable_object_(&sps_pkt); // 发送包
            } else {
              LOG_INFO("av_packet_from_data sps_pkt failed");
            }

            AVPacket pps_pkt = {0};
            pps_pkt.size = pps_vector_[0].size();
            pps_pkt.data = (uint8_t *)av_malloc(pps_pkt.size);
            if (av_packet_from_data(&pps_pkt, pps_pkt.data, pps_pkt.size) ==
                0) {
              memcpy(pps_pkt.data, (uint8_t *)pps_vector_[0].c_str(),
                     pps_vector_[0].size());
              if (!rtmp_dump_h264) {
                rtmp_dump_h264 = fopen("rtmp.h264", "wb+");
              }
              fwrite(pps_pkt.data, pps_pkt.size, 1, rtmp_dump_h264);
              fflush(rtmp_dump_h264);
              if (video_packet_callable_object_)
                video_packet_callable_object_(&pps_pkt); // 发送包
            } else {
              LOG_INFO("av_packet_from_data pps_pkt failed");
            }
            firt_entry = true;
          } else {
            if (keyframe && !is_got_video_iframe_) {
              is_got_video_iframe_ = true;
              LOG_INFO("%s:%s:t:%u", play_time->getRtmpTag(),
                       play_time->getAvcIFrameTag(),
                       play_time->getCurrenTime());
            }
            if (got_video_frames_++ < PRINT_MAX_FRAMES) {
              // 打印前PRINT_MAX_FRAMES帧的时间信息，包括i帧
              LOG_INFO("%s:%s:k:%d:t:%u", play_time->getRtmpTag(),
                       play_time->getAvcFrameTag(), keyframe,
                       play_time->getCurrenTime());
            }
            uint32_t duration = video_frame_duration_;
            // 计算pts
            if (video_pre_pts_ == -1) {
              video_pre_pts_ = packet.m_nTimeStamp;
              if (!packet.m_hasAbsTimestamp) {
                LOG_INFO("no init video pts");
              }
            } else {
              if (packet.m_hasAbsTimestamp) {
                video_pre_pts_ = packet.m_nTimeStamp;
              } else {
                duration = packet.m_nTimeStamp;
                video_pre_pts_ += packet.m_nTimeStamp;
              }
            }
            LOG_INFO("vpts:%u, t:%u", video_pre_pts_, packet.m_nTimeStamp);

            uint32_t offset = 5;
            total_slice_size = 0;
            total_slice_offset = 0;
            int first_slice = 1;
            while (offset < packet.m_nBodySize) {
              uint8_t ch0 = packet.m_body[offset];
              uint8_t ch1 = packet.m_body[offset + 1];
              uint8_t ch2 = packet.m_body[offset + 2];
              uint8_t ch3 = packet.m_body[offset + 3];
              uint32_t data_len =
                  ((ch0 << 24) | (ch1 << 16) | (ch2 << 8) | ch3);
              memcpy(&packet.m_body[offset], nalu_header_4bytes, 4);
              offset += 4; // 跳过data_len占用的4字节
              uint8_t nalu_type = 0x1f & packet.m_body[offset];
              //                        LOG_INFO("nalu_type:%d", nalu_type);
              if (first_slice) {
                first_slice = 0;
                memcpy(&nalu_buf[total_slice_offset], nalu_header_4bytes, 4);
                total_slice_offset += 4;
              } else {
                memcpy(&nalu_buf[total_slice_offset], nalu_header_3bytes, 3);
                total_slice_offset += 3;
              }

              memcpy(&nalu_buf[total_slice_offset], &packet.m_body[offset],
                     data_len);
              offset += data_len; // 跳过data_len
              total_slice_offset += data_len;
              first_slice = 0;
            }
            {
              AVPacket nalu_pkt = {0};
              nalu_pkt.size = total_slice_offset;
              nalu_pkt.data = (uint8_t *)av_malloc(nalu_pkt.size);

              if (av_packet_from_data(&nalu_pkt, nalu_pkt.data,
                                      nalu_pkt.size) == 0) {
                //                            memcpy(&nalu_pkt.data[0],
                //                            nalu_header_4bytes, 4);
                memcpy(nalu_pkt.data, (uint8_t *)nalu_buf, total_slice_offset);

                nalu_pkt.duration = duration;
                nalu_pkt.dts = video_pre_pts_;
                if (keyframe)
                  nalu_pkt.flags = AV_PKT_FLAG_KEY;
                if (!rtmp_dump_h264) {
                  rtmp_dump_h264 = fopen("rtmp.h264", "wb+");
                }
                fwrite(nalu_pkt.data, nalu_pkt.size, 1, rtmp_dump_h264);
                fflush(rtmp_dump_h264);
                if (video_packet_callable_object_)
                  video_packet_callable_object_(&nalu_pkt); // 发送包
              } else {
                LOG_ERROR("av_packet_from_data nalu_pkt failed");
              }
            }
          }
        } else if (packet.m_packetType == RTMP_PACKET_TYPE_AUDIO) {
          static int64_t s_is_pre_ready = TimesUtil::GetTimeMillisecond();
          cur_time = TimesUtil::GetTimeMillisecond();
          s_is_pre_ready = cur_time;

          bool sequence = (0x00 == packet.m_body[1]);
          uint8_t format = 0, samplerate = 0, sampledepth = 0, type = 0;
          uint8_t frame_length_flag = 0, depend_on_core_coder = 0,
                  extension_flag = 0;
          // AAC sequence
          if (sequence) {
            LOG_INFO("%s:%s:t:%u", play_time->getRtmpTag(),
                     play_time->getAacHeaderTag(), play_time->getCurrenTime());

            format = (packet.m_body[0] & 0xf0) >> 4;
            samplerate = (packet.m_body[0] & 0x0c) >> 2;
            sampledepth = (packet.m_body[0] & 0x02) >> 1;
            type = packet.m_body[0] & 0x01;
            // sequence = packet.m_body[1];
            // AAC(AudioSpecificConfig)
            if (format == 10) { // AAC格式
              uint8_t ch0 = packet.m_body[2];
              uint8_t ch1 = packet.m_body[3];
              uint16_t config = ((ch0 << 8) | ch1);
              profile_ = (config & 0xF800) >> 11;
              sample_frequency_index_ = (config & 0x0780) >> 7;
              //                        sample_frequency_index_ = 5;
              channels_ = (config & 0x78) >> 3;
              frame_length_flag = (config & 0x04) >> 2;
              depend_on_core_coder = (config & 0x02) >> 1;
              extension_flag = config & 0x01;
            }
            // Speex(Fix data here, so no need to parse...)
            else if (format == 11) { // MP3格式
              // 16 KHz, mono, 16bit/sample
              type = 0;
              sampledepth = 1;
              samplerate = 4;
            }
            audio_sample_rate = getSampleRateByFreqIdx(sample_frequency_index_);
          }
          // Audio frames
          else {
            if (got_audio_frames_++ < PRINT_MAX_FRAMES) {
              // 打印前PRINT_MAX_FRAMES帧的时间信息，包括i帧
              LOG_INFO("%s:%s:t:%u", play_time->getRtmpTag(),
                       play_time->getAacDataTag(), play_time->getCurrenTime());
            }
            uint32_t duration = audio_frame_duration_;
            if (audio_pre_pts_ == -1) {
              audio_pre_pts_ = packet.m_nTimeStamp;
              if (!packet.m_hasAbsTimestamp) {
                LOG_INFO("no init video pts");
              }
            } else {
              if (packet.m_hasAbsTimestamp)
                audio_pre_pts_ = packet.m_nTimeStamp;
              else {
                duration = packet.m_nTimeStamp;
                audio_pre_pts_ += packet.m_nTimeStamp;
              }
            }
            LOG_INFO("apts:%u, t:%u", audio_pre_pts_, packet.m_nTimeStamp);
            // ADTS(7 bytes) + AAC data
            uint32_t data_len = packet.m_nBodySize - 2 + 7;
            uint8_t adts[7];
            adts[0] = 0xff;
            adts[1] = 0xf9;
            adts[2] = ((profile_ - 1) << 6) | (sample_frequency_index_ << 2) |
                      (channels_ >> 2);
            adts[3] = ((channels_ & 3) << 6) + (data_len >> 11);
            adts[4] = (data_len & 0x7FF) >> 3;
            adts[5] = ((data_len & 7) << 5) + 0x1F;
            adts[6] = 0xfc;

            AVPacket aac_pkt = {0};
            aac_pkt.size = data_len;
            aac_pkt.data = (uint8_t *)av_malloc(aac_pkt.size);
            if (av_packet_from_data(&aac_pkt, aac_pkt.data, aac_pkt.size) ==
                0) {
              // 带 adts header
              memcpy(&aac_pkt.data[0], adts, 7);
              memcpy(&aac_pkt.data[7], packet.m_body + 2,
                     packet.m_nBodySize - 2);
              aac_pkt.duration = duration;
              aac_pkt.dts = audio_pre_pts_;
              LOG_DEBUG("rtmp adts:%ld", aac_pkt.dts);
              if (audio_packet_callable_object_)
                audio_packet_callable_object_(&aac_pkt); // 发送包
              static FILE *rtmp_dump_aac = NULL;
              if (!rtmp_dump_aac) {
                rtmp_dump_aac = fopen("rtmp.aac", "wb+");
              }
              fwrite(aac_pkt.data, aac_pkt.size, 1, rtmp_dump_aac);
              fflush(rtmp_dump_aac);
            } else {
              LOG_ERROR("av_packet_from_data aac_pkt failed");
            }
          }
        } else if (packet.m_packetType == RTMP_PACKET_TYPE_INFO) {
          LOG_INFO("%s:%s:t:%u", play_time->getRtmpTag(),
                   play_time->getMetadataTag(), play_time->getCurrenTime());
          is_got_metadta_ = true;
          parseScriptTag(packet);
        } else {
          LOG_INFO("can't handle it ");
          rtmpProtocol_->respondPacket(&packet);
        }
        rtmpProtocol_->freePacket(&packet);
        memset(&packet, 0, sizeof(RTMPPacket));
      } else {
        continue;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
  free(nalu_buf);
  std::cout << "thread exit" << std::endl;
  sync_thread_.notify_one();
}

uint32_t RTMPPuller::getSampleRateByFreqIdx(uint8_t freq_idx) {
  uint32_t freq_idx_table[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                               22050, 16000, 12000, 11025, 8000,  7350};
  if (freq_idx < 13) {
    return freq_idx_table[freq_idx];
  }
  LOG_INFO("freq_idx:%d is error", freq_idx);
  return 44100;
}

} // namespace TransProtocol
