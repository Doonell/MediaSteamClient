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

template <typename Container>
class RTMPPusher
    : public Middleware::BaseTrigger<FLVMetaMessage, VideoSequenceMessage,
                                     H264RawMessage, AudioSpecificConfigMessage,
                                     AudioRawDataMessage>,
      public std::enable_shared_from_this<RTMPPusher<Container>> {
public:
  RTMPPusher(Container &msgQueue,
             const std::shared_ptr<Encoder::AACEncoder> &audioEncoder,
             const std::shared_ptr<Encoder::H264Encoder> &videoEncoder,
             const std::shared_ptr<Encoder::AudioS16Resampler> &audioResampler)
      : msgQueue_(msgQueue), audioEncoder_(audioEncoder),
        videoEncoder_(videoEncoder), audioResampler_(audioResampler),
        rtmpProtocol_(std::make_shared<RTMPProtocol>(
            "rtmp://101.37.125.75/live/livestream")) {}
  ~RTMPPusher() = default;

  void start() {
    AVPublishTime::GetInstance()->Rest();
    rtmpProtocol_->connect();
    msgQueue_.addReceiver(
        std::static_pointer_cast<Middleware::IReceiver>(shared_from_this()));
    msgQueue_.delegate();
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

    videoEncoder_->encode(yuv, [&](AVPacket &packet) {
      if (packet.size > 0) {
        int nalu_buf_size = 0;
        std::shared_ptr<uint8_t[]> nalu_buf(
            new uint8_t[VIDEO_NALU_BUF_MAX_SIZE]);
        memcpy(nalu_buf.get(), packet.data + 4, packet.size - 4);
        nalu_buf_size = packet.size - 4;
        auto pts = AVPublishTime::GetInstance()->get_video_pts();
        auto nalu_type = nalu_buf[0] & 0x1f;
        auto rawData = std::make_shared<Message::H264RawMessage>(
            nalu_buf, nalu_buf_size, nalu_type, pts);
        msgQueue_.publish(rawData);
      }
    });
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

    for (int i = 0; i < resampled_frames.size(); i++) {
      int sizeEncoded = audioEncoder_->encode(
          resampled_frames[i].get(), [&](ACPacket &packet) {
            if (packet.size > 0) {
              auto rawData = std::make_shared<Message::AudioRawDataMessage>(
                  packet.size + 2);
              rawData->pts = AVPublishTime::GetInstance()->get_audio_pts();
              rawData->data_[0] = 0xaf;
              rawData->data_[1] = 0x01;
              memcpy(&rawData->data_[2], packet.data, packet.size);
              msgQueue_.publish(rawData);
            }
          });
    }
  }

  bool sendMetadata(const FLVMetaMessage &metadata);

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
    metadata->audiosamplesize = 16; // need to be checked
    metadata->audiodatarate = 64;
    metadata->pts = 0;
    msgQueue_.publish(metadata);
  }

  void scenario<VideoSequenceMessage>::handle(
      const VideoSequenceMessage &t) override {
    handleMessage(t);
  }

  void scenario<H264RawMessage>::handle(const H264RawMessage &t) override {
    handleMessage(t);
  }

  void scenario<AudioSpecificConfigMessage>::handle(
      const AudioSpecificConfigMessage &t) override {
    handleMessage(t);
  }

  void
  scenario<AudioRawDataMessage>::handle(const AudioRawDataMessage &t) override {
    handleMessage(t);
  }

  void scenario<FLVMetaMessage>::handle(const FLVMetaMessage &t) override {
    rtmpProtocol_->sendMetaData(
        t->width, t->height, t->framerate, t->videodatarate, t->audiodatarate,
        t->audiosamplerate, t->audiosamplesize, t->channles);
  }

private:
  void handleMessage(const VideoSequenceMessage &msg) {
    std::cout << "handleMessage : VideoSequenceMessage" << std::endl;
    rtmpProtocol_->sendH264SequenceHeader(msg->sps_, msg->sps_size_, msg->pps_,
                                          msg->pps_size_);
  }

  void handleMessage(const H264RawMessage &msg) {
    rtmpProtocol_->sendH264RawData(msg->isKeyFrame(), msg->getData(),
                                   msg->getSize(), msg->pts());
  }

  void handleMessage(const AudioSpecificConfigMessage &msg) {
    rtmpProtocol_->sendAudioSpecificConfig(msg->profile_ + 1, msg->channels_,
                                           msg->sample_rate_);
  }

  void handleMessage(const AudioRawDataMessage &msg) {
    rtmpProtocol_->sendAudioRawData(msg->data_, msg->size_, msg->pts);
  }

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
