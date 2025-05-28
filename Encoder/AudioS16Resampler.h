#ifdef __AUDIO_S16_RESAMPLER_H__
#define __AUDIO_S16_RESAMPLER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/audio_fifo.h"     // av_audio_fifo_alloc
#include "libavutil/avutil.h"         //av_rescale_rnd in mathmatics.h
#include "libavutil/opt.h"            // av_set_int
#include "libswresample/swresample.h" //swr_alloc, swr_init
#ifdef __cplusplus
};
#endif

namespace Encoder {
class AudioS16Resampler {
public:
  AudioS16Resampler(int srcChannels, int dstChannels, int srcSampleRate,
                    int dstSampleRate, int srcSampleFormat, int dstSampleFormat,
                    int srcChannelsLayout, int dstChannelsLayout);
  ~AudioS16Resampler() = default;

  bool init();
  int resample(const uint8_t *in_data, int in_size, uint8_t *out_data,
               int out_size);

private:
  int srcChannels_;
  int dstChannels_;
  int srcSampleRate_;
  int dstSampleRate_;
  int srcSampleFormat_;
  int dstSampleFormat_;
  int srcChannelsLayout_;
  int dstChannelsLayout_;

  int total_resampled_num_ = 0;
  int64_t start_pts_ = AV_NOPTS_VALUE;
  int64_t cur_pts_ = AV_NOPTS_VALUE;
  int dst_sample_fmt_ = 0;
  int dst_channel_layout_ = 0;
  int dst_sample_format_ = 0;
  int dst_channels_ = 2;
  int src_nb_samples = 1024;
  int dst_nb_samples = 0;
  int max_dst_nb_samples_ = 0;
  AVAudioFifo *audioFifo_ = nullptr;
  uint8_t **resampled_data_ = nullptr;
  int dst_linesize = 0;
};
} // namespace Encoder
#endif // __AUDIO_S16_RESAMPLER_H__