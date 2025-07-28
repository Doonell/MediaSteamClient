#ifndef _H264_ENCODER_H_
#define _H264_ENCODER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavcodec/version.h>
#include <libavutil/opt.h>
#ifdef __cplusplus
};
#endif
#include <functional>
#include <string>
namespace Encoder {

class H264Encoder {
public:
  H264Encoder(int width = 720, int height = 480, int fps = 25,
              int bitrate = 512 * 1024, int gop = 25, int b_frames = 0);
  ~H264Encoder();
  bool init();

  template <typename H264Callback>
  void encode(uint8_t *in, H264Callback handleH264) {
    frame_->data[0] = in;                      // Y
    frame_->data[1] = in + data_size_;         // U
    frame_->data[2] = in + data_size_ * 5 / 4; // V
    frame_->pts = (count++) * (ctx_->time_base.den) /
                  ((ctx_->time_base.num) * 25); // 时间戳
    av_init_packet(&packet_);
    // Encode
    int got_picture = 0;
    int ret = avcodec_encode_video2(ctx_, &packet_, frame_, &got_picture);

    if (ret < 0) {
      std::cout << "Failed to encode!" << std::endl;
      return;
    }

    if (got_picture == 1) {
      framecnt++;
      // rtmp-flv不带4字节的NALU头00,00,00,01
      // memcpy(out.get(), packet_.data + 4, packet_.size - 4);
      // out_size = packet_.size - 4;
      handleH264(packet_);
      av_packet_unref(&packet_); // 释放内存 不释放则内存泄漏
    }
    else {
        std::cout << "Got picture is not 1, got_picture: " << got_picture
            << std::endl;
    }
  }

  const AVCodec *find_encoder_by_name(const std::string &codec_name);
  inline int get_width() { return ctx_->width; }
  inline int get_height() { return ctx_->height; }
  double get_framerate() {
    return ctx_->framerate.num / ctx_->framerate.den; // den=1, num=25
  }
  inline int64_t get_bit_rate() { return ctx_->bit_rate; }
  inline uint8_t *get_sps_data() { return (uint8_t *)sps_.c_str(); }
  inline int get_sps_size() { return sps_.size(); }
  inline uint8_t *get_pps_data() { return (uint8_t *)pps_.c_str(); }
  inline int get_pps_size() { return pps_.size(); }
  AVCodecContext *getCodecContext() { return ctx_; }

private:
  int width_;
  int height_;
  int fps_;
  int bitrate_;
  int gop_;
  int b_frames_;
  int count = 0;
  int data_size_;
  int framecnt;

  std::string codec_name_; //
  std::string profile_;
  std::string level_id_;

  std::string sps_;
  std::string pps_;
  // data
  AVFrame *frame_ = nullptr;
  uint8_t *picture_buf_ = nullptr;
  AVPacket packet_;

  // encoder message
  AVCodec *codec_ = nullptr;
  AVDictionary *param = nullptr;
  AVCodecContext *ctx_ = nullptr;

  int64_t pts_ = 0;
  const int VIDEO_NALU_BUF_MAX_SIZE = 1024 * 1024;
};
} // namespace Encoder
#endif // _H264_ENCODER_H_
