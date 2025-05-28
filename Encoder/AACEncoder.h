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
class AACEncoder {
public:
  AACEncoder(int sample_rate, int channels, int bitrate, int channel_layout);
  ~AACEncoder() = default;
  bool init();
  int encode(AVFrame *frame, uint8_t *out, int out_len);

private:
  int bitrate_;        // 默认out_samplerate*3
  int channel_layout_; // 默认AV_CH_LAYOUT_STEREO
  int channels_;       // 默认 2
  int sample_rate_;    // 默认 48000

  AVCodec *codec_;
  AVCodecContext *ctx_;
  AVFrame *frame_;
};

#endif // __AACENCODER_H__
