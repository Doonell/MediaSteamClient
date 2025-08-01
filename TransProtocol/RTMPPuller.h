#ifndef RTMPPLAYER_H
#define RTMPPLAYER_H
#include "RTMPProtocol.h"
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>

namespace TransProtocol {

class RTMPPuller {
public:
  RTMPPuller(std::string url = "rtmp://192.168.133.129/live/livestream");
  ~RTMPPuller();
  void start();
  void stop();

  void readPacketThread();
  void AddAudioInfoCallback(
      std::function<void(uint8_t, uint8_t, uint8_t, uint32_t, int64_t)>
          callback) {
    audio_info_callback_ = callback;
  }

  void AddAudioPacketCallback(std::function<void(void *)> callback) {
    audio_packet_callable_object_ = callback;
  }

  void AddVideoPacketCallback(std::function<void(void *)> callback) {
    video_packet_callable_object_ = callback;
  }

  // 收到视频数据包调用回调
  void AddVideoInfoCallback(std::function<void(int, int, int, bool)> callback) {
    video_info_callback_ = callback;
  }

  uint32_t getSampleRateByFreqIdx(uint8_t freq_idx);

private:
  std::string url_;
  void parseScriptTag(RTMPPacket &packet);
  bool request_exit_thread_ = false;
  std::thread worker_;
  std::function<void(uint8_t, uint8_t, uint8_t, uint32_t, int64_t)>
      audio_info_callback_;
  std::function<void(int, int, int, bool)> video_info_callback_;

  std::function<void(void *)> audio_packet_callable_object_;
  std::function<void(void *)> video_packet_callable_object_;

private:
  std::shared_ptr<IRTMPProtocol> rtmpProtocol_;
  // video and audio info
  int video_codec_id = 0;
  int video_width = 0;
  int video_height = 0;
  int video_frame_rate = 0;
  int audio_codec_id = 0;
  int audio_sample_rate = 0;
  int audio_bit_rate = 0;
  int audio_sample_size = 0;
  int audio_channel = 2;
  int file_size = 0;

  uint32_t video_frame_duration_ = 40; // 默认是40毫秒
  uint32_t audio_frame_duration_ = 21; // 默认是21毫秒 aac 48kh

  uint8_t profile_ = 0;
  uint8_t sample_frequency_index_ = 0;
  uint8_t channels_ = 0;
  std::vector<std::string> sps_vector_; // 可以存储多个sps
  std::vector<std::string> pps_vector_; // 可以存储多个pps

  int64_t audio_pre_pts_ = -1;
  int64_t video_pre_pts_ = -1;

  // 性能指标统计
  uint32_t PRINT_MAX_FRAMES =
      30; // 打印前xx帧，根据实际调试情况进行修改，如果打印太多影响性能
  bool is_got_metadta_ = false;
  bool is_got_video_sequence_ = false; // video sequence
  bool is_got_video_iframe_ = false;   // 打印第一次收到i帧
  uint32_t got_video_frames_ = 0; // 对接收的video帧进行计数, 打印前xx帧的时间

  bool is_got_audio_sequence_ = false;
  uint32_t got_audio_frames_ = 0; //

  bool firt_entry = false;
  std::mutex mutex_;
  std::condition_variable sync_thread_;
};
} // namespace TransProtocol

#endif // RTMPPLAYER_H
