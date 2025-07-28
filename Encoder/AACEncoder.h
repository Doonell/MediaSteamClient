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

// 最大为13bit长度(8191), +64 只是防止字节对齐
const int AAC_BUF_MAX_LENGTH = 8291 + 64;

class AACEncoder {
public:
  AACEncoder(int sample_rate = 48000, int channels = 2,
             int bitrate = 128 * 1024, int channel_layout = 3);
  ~AACEncoder();
  bool init();

  template <typename AACCallback>
  void encode(AVFrame *frame, const AACCallback &handleAACcallback) {
    int got_output = 0;
    std::shared_ptr<uint8_t[]> out(new uint8_t[AAC_BUF_MAX_LENGTH]);

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = out.get();
    pkt.size = AAC_BUF_MAX_LENGTH;

    if (avcodec_encode_audio2(ctx_, &pkt, frame, &got_output) < 0) {
      std::cout << "Error encoding audio" << std::endl;
      return;
    }

    if (!got_output) {
      std::cout << "AAC: could not get output packet" << std::endl;
      return;
    }

    handleAACcallback(pkt);
    av_packet_unref(&pkt);
  }

  int get_sample_rate() { return ctx_->sample_rate; }
  int get_profile() { return ctx_->profile; }
  int get_channels() { return ctx_->channels; }
  uint32_t GetFrameSampleSize() { return ctx_->frame_size; }
  AVCodecContext *getCodecContext() { return ctx_; }

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
