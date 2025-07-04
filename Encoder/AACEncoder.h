#ifndef __AACENCODER_H__
#define __AACENCODER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#ifdef __cplusplus
};
#endif

namespace Encoder {
class AACEncoder {
public:
  AACEncoder(int sample_rate = 48000, int channels = 2,
             int bitrate = 128 * 1024, int channel_layout = 3);
  ~AACEncoder();
  bool init();
  int encode(AVFrame *frame, uint8_t *out, int out_len);

  int get_sample_rate() { return ctx_->sample_rate; }
  int get_profile() { return ctx_->profile; }
  int get_channels() { return ctx_->channels; }
  uint32_t GetFrameSampleSize() { return ctx_->frame_size; }

private:
  int sample_rate_;    // 默认 48000
  int channels_;       // 默认 2
  int bitrate_;        // 默认out_samplerate*3
  int channel_layout_; // 默认AV_CH_LAYOUT_STEREO

  AVCodec *codec_;
  AVCodecContext *ctx_;
  AVFrame *frame_;
};
} // namespace Encoder
#endif // __AACENCODER_H__
