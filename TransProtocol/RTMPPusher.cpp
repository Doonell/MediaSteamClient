#include "RTMPPusher.h"

namespace TransProtocol {

// template <typename Container>
// void RTMPPusher<Container>::sendAudioPacket(uint8_t *pcm, int size) {
//   if (metaDataSent_) {
//     sendSetMetaData();
//     metaDataSent_ = false;
//   }

//   if (need_send_audio_spec_config) {
//     need_send_audio_spec_config = false;
//     auto audioSpecificConfigMessage =
//         std::make_shared<Message::AudioSpecificConfigMessage>(
//             audioEncoder_->get_profile(), audioEncoder_->get_channels(),
//             audioEncoder_->get_sample_rate());
//     msgQueue_.publish(audioSpecificConfigMessage);
//   }

//   if (need_send_video_config) {
//     need_send_video_config = false;
//     auto vid_config_msg = std::make_shared<Message::VideoSequenceMessage>(
//         videoEncoder_->get_sps_data(), videoEncoder_->get_sps_size(),
//         videoEncoder_->get_pps_data(), videoEncoder_->get_pps_size());
//     vid_config_msg->nWidth = video_width_;
//     vid_config_msg->nHeight = video_height_;
//     vid_config_msg->nFrameRate = video_fps_;
//     vid_config_msg->nVideoDataRate = video_bitrate_;
//     vid_config_msg->pts_ = 0;
//     msgQueue_.publish(vid_config_msg);
//   }

//   auto ret = audioResampler_->sendFrame(pcm, size);
//   if (ret < 0) {
//     LogError("sendFrame failed ");
//     return;
//   }

//   std::vector<std::shared_ptr<AVFrame>> resampled_frames;
//   ret = audioResampler_->receiveFrame(resampled_frames,
//                                       audioEncoder_->GetFrameSampleSize());
//   if (!ret) {
//     LogWarn("receiveFrame ret:%d\n", ret);
//     return;
//   }

//   for (int i = 0; i < resampled_frames.size(); i++) {
//     int sizeEncoded = audio_encoder_->encode(resampled_frames[i].get(),
//                                              aac_buf_, AAC_BUF_MAX_LENGTH);
//     if (sizeEncoded > 0) {
//       auto rawData =
//           std::make_shared<Message::AudioRawDataMessage>(sizeEncoded + 2);
//       rawData->pts = AVPublishTime::GetInstance()->get_audio_pts();
//       rawData->data[0] = 0xaf;
//       rawData->data[1] = 0x01;
//       memcpy(&rawData->data[2], aac_buf_, sizeEncoded);
//       msgQueue_.publish(rawData);
//     }
//   }
// }

// template <typename Container> void RTMPPusher<Container>::sendSetMetaData() {
//   // RTMP -> FLV的格式去发送， metadata
//   auto metadata = std::make_shared<FLVMetaMessage>();
//   // 设置视频相关
//   metadata->has_video = true;
//   metadata->width = videoEncoder_->get_width();
//   metadata->height = videoEncoder_->get_height();
//   metadata->framerate = videoEncoder_->get_framerate();
//   metadata->videodatarate = videoEncoder_->get_bit_rate();
//   // 设置音频相关
//   metadata->has_audio = true;
//   metadata->channles = audioEncoder_->get_channels();
//   metadata->audiosamplerate = audioEncoder_->get_sample_rate();
//   metadata->audiosamplesize = 16;
//   metadata->audiodatarate = 64;
//   metadata->pts = 0;
//   msgQueue_.publish(metadata);
// }

template <typename Container>
bool RTMPPusher<Container>::sendMetadata(const FLVMetaMessage &metadata) {
  return rtmpProtocol_->sendMetaData(
      metadata->width, metadata->height, metadata->framerate,
      metadata->videodatarate, metadata->audiodatarate,
      metadata->audiosamplerate, metadata->audiosamplesize, metadata->channels);
}

} // namespace TransProtocol