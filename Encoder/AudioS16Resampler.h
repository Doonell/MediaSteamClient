#ifndef __AUDIO_S16_RESAMPLER_H__
#define __AUDIO_S16_RESAMPLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avutil.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#ifdef __cplusplus
};
#endif

#include <memory>
#include <vector>

namespace Encoder {
class AudioS16Resampler {
public:
  AudioS16Resampler(int srcSampleRate = 48000, int dstSampleRate = 48000,
                    AVSampleFormat srcSampleFormat = AV_SAMPLE_FMT_S16,
                    AVSampleFormat dstSampleFormat = AV_SAMPLE_FMT_FLTP,
                    int srcChannelsLayout = 3, int dstChannelsLayout = 3);
  ~AudioS16Resampler();

  bool init();
  int resample(const uint8_t *in_data, int in_size, uint8_t *out_data,
               int out_size);
  int sendFrame(uint8_t *in_pcm, const int in_size);
  bool receiveFrame(std::vector<std::shared_ptr<AVFrame>> &frames,
                    int desired_size);
  std::shared_ptr<AVFrame> readFrameFromFifo(const int desiredSize);
  std::shared_ptr<AVFrame> createFrameBySamples(const int nb_samples);

private:
  int src_channels_;
  int dst_channels_;
  int src_sample_rate_;
  int dst_sample_rate_;
  AVSampleFormat src_sample_format_;
  AVSampleFormat dst_sample_format_;
  int src_channels_layout_;
  int dst_channels_layout_;

  struct SwrContext *swr_ctx_ = nullptr;
  int total_resampled_num_ = 0;
  int resampled_data_size = 8192;
  int64_t start_pts_ = AV_NOPTS_VALUE;
  int64_t cur_pts_ = AV_NOPTS_VALUE;
  int dst_sample_fmt_ = 0;
  int dst_channel_layout_ = 0;
  int dst_nb_samples = 1024;
  int max_dst_nb_samples_ = 1024;
  AVAudioFifo *audioFifo_ = nullptr;
  uint8_t **resampled_data_ = nullptr;
  int dst_linesize = 0;
};
} // namespace Encoder
#endif // __AUDIO_S16_RESAMPLER_H__