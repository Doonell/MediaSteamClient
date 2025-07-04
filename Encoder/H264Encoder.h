#ifndef _H264_ENCODER_H_
#define _H264_ENCODER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavcodec/version.h>
#ifdef __cplusplus
};
#endif
#include <string>

namespace Encoder {

class H264Encoder {
public:
  H264Encoder(int width = 1920, int height = 1080, int fps = 25,
              int bitrate = 500 * 1024, int gop = 30, int b_frames = 0);
  ~H264Encoder();
  bool init();
  int encode(uint8_t *in, uint32_t in_samples, uint8_t *out,
             uint32_t &out_size);
  void initByName();
  void print_all_encoders();
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

private:
  int width_;
  int height_;
  int fps_;
  int bitrate_;
  int gop_;
  int b_frames_;
  int count;
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
};
} // namespace Encoder
#endif // _H264_ENCODER_H_
