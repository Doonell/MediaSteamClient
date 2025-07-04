#ifndef _RTMP_PUSHER_H_
#define _RTMP_PUSHER_H_

#include "../Encoder/AACEncoder.h"
#include "../Encoder/AudioS16Resampler.h"
#include "../Encoder/H264Encoder.h"
#include "../Message/VideoMessage.h"
#include "../Middleware/MsgQueue.h"
#include "../Util/TimeHelper.h"
#include "AVTagDataBuilder.h"
#include "RTMPMessageFormatBuilder.h"
#include "RTMPPacketBuilder.h"
#include "RTMPProtocol.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace TransProtocol {
// 最大为13bit长度(8191), +64 只是防止字节对齐
const int AAC_BUF_MAX_LENGTH = 8291 + 64;
template <typename Container> class RTMPPusher {
public:
  RTMPPusher(Container &msgQueue,
             const std::shared_ptr<Encoder::AACEncoder> &audioEncoder,
             const std::shared_ptr<Encoder::H264Encoder> &videoEncoder,
             const std::shared_ptr<Encoder::AudioS16Resampler> &audioResampler)
      : msgQueue_(msgQueue), audioEncoder_(audioEncoder),
        videoEncoder_(videoEncoder), audioResampler_(audioResampler),
        rtmpProtocol_(
            std::make_shared<RTMPProtocol>("rtmp://192.168.133.129/live", 1)) {}
  ~RTMPPusher() = default;

  void start() {
    AVPublishTime::GetInstance()->Rest();
    auto msg = msgQueue_.subscribe<VideoSequenceMessage>(
        [&](auto &msg) { handleMessage(msg); });
    rtmpProtocol_->connect(RtmpUrl);
  }

  void sendVideoPacket(uint8_t *yuv, int size) {
    if (metaDataSent_) {
      sendSetMetaData();
      metaDataSent_ = false;
    }

    if (need_send_video_config) {
      need_send_video_config = false;
      auto vid_config_msg = std::make_shared<Message::VideoSequenceMessage>(
          videoEncoder_->get_sps_data(), videoEncoder_->get_sps_size(),
          videoEncoder_->get_pps_data(), videoEncoder_->get_pps_size());
      vid_config_msg->nWidth = video_width_;
      vid_config_msg->nHeight = video_height_;
      vid_config_msg->nFrameRate = video_fps_;
      vid_config_msg->nVideoDataRate = video_bitrate_;
      vid_config_msg->pts_ = 0;
      msgQueue_.publish(vid_config_msg);
    }

    std::shared_ptr<uint8_t[]> video_nalu_buf(
        new uint8_t[VIDEO_NALU_BUF_MAX_SIZE]);
    video_nalu_size_ = VIDEO_NALU_BUF_MAX_SIZE;
    int sizeEncoded =
        videoEncoder_->encode(yuv, 0, video_nalu_buf.get(), video_nalu_size_);
    if (sizeEncoded > 0) {
      auto pts = AVPublishTime::GetInstance()->get_video_pts();
      auto nalu_type = video_nalu_buf[0] & 0x1f;
      auto rawData = std::make_shared<Message::H264RawMessage>(
          video_nalu_buf, video_nalu_size_, nalu_type, pts);
      msgQueue_.publish(rawData);
    }
  }

  void sendAudioPacket(uint8_t *pcm, int size) {
    if (metaDataSent_) {
      sendSetMetaData();
      metaDataSent_ = false;
    }

    if (need_send_audio_spec_config) {
      need_send_audio_spec_config = false;
      auto audioSpecificConfigMessage =
          std::make_shared<Message::AudioSpecificConfigMessage>(
              audioEncoder_->get_profile(), audioEncoder_->get_channels(),
              audioEncoder_->get_sample_rate());
      msgQueue_.publish(audioSpecificConfigMessage);
    }

    if (need_send_video_config) {
      need_send_video_config = false;
      auto vid_config_msg = std::make_shared<Message::VideoSequenceMessage>(
          videoEncoder_->get_sps_data(), videoEncoder_->get_sps_size(),
          videoEncoder_->get_pps_data(), videoEncoder_->get_pps_size());
      vid_config_msg->nWidth = video_width_;
      vid_config_msg->nHeight = video_height_;
      vid_config_msg->nFrameRate = video_fps_;
      vid_config_msg->nVideoDataRate = video_bitrate_;
      vid_config_msg->pts_ = 0;
      msgQueue_.publish(vid_config_msg);
    }

    auto ret = audioResampler_->sendFrame(pcm, size);
    if (ret < 0) {
      LOG_INFO("sendFrame failed ");
      return;
    }

    std::vector<std::shared_ptr<AVFrame>> resampled_frames;
    ret = audioResampler_->receiveFrame(resampled_frames,
                                        audioEncoder_->GetFrameSampleSize());
    if (!ret) {
      LOG_INFO("receiveFrame ret:%d\n", ret);
      return;
    }

    std::array<uint8_t, AAC_BUF_MAX_LENGTH> aac_buf_;
    for (int i = 0; i < resampled_frames.size(); i++) {
      int sizeEncoded = audioEncoder_->encode(
          resampled_frames[i].get(), aac_buf_.data(), AAC_BUF_MAX_LENGTH);
      if (sizeEncoded > 0) {
        auto rawData =
            std::make_shared<Message::AudioRawDataMessage>(sizeEncoded + 2);
        rawData->pts = AVPublishTime::GetInstance()->get_audio_pts();
        rawData->data[0] = 0xaf;
        rawData->data[1] = 0x01;
        memcpy(&rawData->data[2], aac_buf_.data(), sizeEncoded);
        msgQueue_.publish(rawData);
      }
    }
  }

  bool sendMetadata(FLVMetaMessage &metadata);

  void sendSetMetaData() {
    // RTMP -> FLV的格式去发送， metadata
    auto metadata = std::make_shared<Message::FLVMetaMessage>();
    // 设置视频相关
    metadata->has_video = true;
    metadata->width = videoEncoder_->get_width();
    metadata->height = videoEncoder_->get_height();
    metadata->framerate = videoEncoder_->get_framerate();
    metadata->videodatarate = videoEncoder_->get_bit_rate();
    // 设置音频相关
    metadata->has_audio = true;
    metadata->channles = audioEncoder_->get_channels();
    metadata->audiosamplerate = audioEncoder_->get_sample_rate();
    metadata->audiosamplesize = 16;
    metadata->audiodatarate = 64;
    metadata->pts = 0;
    msgQueue_.publish(metadata);
  }

private:
  void handleMessage(VideoSequenceMessage &msg);
  void handleMessage(H264RawMessage &msg);
  void handleMessage(AudioSpecificConfigMessage &msg);
  void handleMessage(AudioRawDataMessage &msg);

private:
  Container &msgQueue_;
  std::shared_ptr<Encoder::AACEncoder> audioEncoder_;
  std::shared_ptr<Encoder::H264Encoder> videoEncoder_;
  std::shared_ptr<Encoder::AudioS16Resampler> audioResampler_;
  std::shared_ptr<IRTMPProtocol> rtmpProtocol_;

  uint8_t *video_nalu_buf = nullptr;

  bool need_send_audio_spec_config = true;
  bool metaDataSent_ = true;
  bool need_send_video_config = true;
  uint32_t video_nalu_size_ = 0;
  const int VIDEO_NALU_BUF_MAX_SIZE = 1024 * 1024;
  int video_width_ = 1920;
  int video_height_ = 1080;
  int video_fps_ = 25;
  int video_bitrate_ = 1024 * 1024;
  int video_b_frames_ = 0;
  std::string RtmpUrl = "rtmp://192.168.133.129/live/stream";
};

} // namespace TransProtocol

#endif // _RTMP_PUSHER_H_
