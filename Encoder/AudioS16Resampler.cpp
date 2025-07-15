#include "AudioS16Resampler.h"
#include "../Logger/Logger.h"
#include <memory>
#include <vector>
namespace Encoder {

AudioS16Resampler::AudioS16Resampler(int srcSampleRate, int dstSampleRate,
                                     AVSampleFormat srcSampleFormat,
                                     AVSampleFormat dstSampleFormat,
                                     int srcChannelsLayout,
                                     int dstChannelsLayout)
    : src_channels_layout_(srcChannelsLayout),
      dst_channels_layout_(dstChannelsLayout), src_sample_rate_(srcSampleRate),
      dst_sample_rate_(dstSampleRate), src_sample_format_(srcSampleFormat),
      dst_sample_format_(dstSampleFormat) {
  src_channels_ = av_get_default_channel_layout(src_channels_layout_);
  dst_channels_ = av_get_default_channel_layout(dst_channels_layout_);
}

AudioS16Resampler::~AudioS16Resampler() {
  if (swr_ctx_)
    swr_free(&swr_ctx_);
  if (audioFifo_) {
    av_audio_fifo_free(audioFifo_);
    audioFifo_ = nullptr;
  }
  if (resampled_data_)
    av_freep(&resampled_data_[0]);
  av_freep(&resampled_data_);
}

bool AudioS16Resampler::init() {
  src_channels_ = av_get_channel_layout_nb_channels(src_channels_layout_);
  dst_channels_ = av_get_channel_layout_nb_channels(dst_channels_layout_);
  audioFifo_ = av_audio_fifo_alloc(dst_sample_format_, dst_channels_, 1);
  if (!audioFifo_) {
    LOG_ERROR("%s av_audio_fifo_alloc failed", logtag_.c_str());
    return false;
  }

  swr_ctx_ = swr_alloc();
  if (!swr_ctx_) {
    LOG_ERROR("Could not allocate resampling context");
    return false;
  }

  int swrOpts = 0;
  swrOpts |=
      av_opt_set_int(swr_ctx_, "in_channel_layout", src_channels_layout_, 0);
  swrOpts |= av_opt_set_int(swr_ctx_, "in_sample_rate", src_sample_rate_, 0);
  swrOpts |=
      av_opt_set_sample_fmt(swr_ctx_, "in_sample_fmt", src_sample_format_, 0);

  swrOpts |=
      av_opt_set_int(swr_ctx_, "out_channel_layout", dst_channels_layout_, 0);
  swrOpts |= av_opt_set_int(swr_ctx_, "out_sample_rate", dst_sample_rate_, 0);
  swrOpts |=
      av_opt_set_sample_fmt(swr_ctx_, "out_sample_fmt", dst_sample_format_, 0);
  if (swrOpts != 0) {
    LOG_ERROR("Could not set resampling options");
    return false;
  }

  if (swr_init(swr_ctx_) < 0) {
    LOG_ERROR("Could not initialize resampling context");
    return false;
  }
  int src_nb_samples = 1024;
  dst_nb_samples = av_rescale_rnd(src_nb_samples, dst_sample_rate_,
                                  src_sample_rate_, AV_ROUND_UP);
  max_dst_nb_samples_ = dst_nb_samples;
  int ret = av_samples_alloc_array_and_samples(&resampled_data_, &dst_linesize,
                                               dst_channels_, dst_sample_rate_,
                                               dst_sample_format_, 0);
  if (ret < 0) {
    LOG_ERROR("Could not allocate resampled data");
    return false;
  }

  return true;
}

int AudioS16Resampler::sendFrame(uint8_t *in_pcm, const int in_size) {
  auto frame = std::shared_ptr<AVFrame>(
      av_frame_alloc(), [](AVFrame *frame) { av_frame_free(&frame); });
  if (!frame) {
    std::cout << "Could not allocate audio frame" << std::endl;
    return -1;
  }

  frame->format = src_sample_format_;
  frame->channel_layout = src_channels_layout_;
  int ch = av_get_channel_layout_nb_channels(src_channels_layout_);
  frame->nb_samples =
      in_size / av_get_bytes_per_sample(src_sample_format_) / ch;
  // 内部还参考了nb_samples
  avcodec_fill_audio_frame(frame.get(), ch, src_sample_format_, in_pcm, in_size,
                           0);

  int src_nb_samples = frame->nb_samples;
  uint8_t **src_data = frame->extended_data;
  if (start_pts_ == AV_NOPTS_VALUE && frame->pts != AV_NOPTS_VALUE) {
    start_pts_ = frame->pts;
    cur_pts_ = frame->pts;
  }

  int delay = swr_get_delay(swr_ctx_, src_sample_rate_);
  int dst_nb_samples = av_rescale_rnd(delay + src_nb_samples, dst_sample_rate_,
                                      src_sample_rate_, AV_ROUND_UP);
  if (dst_nb_samples > max_dst_nb_samples_) {
    std::cout << "dst_nb_samples is too large" << std::endl;
    return -1;
  }

  int nb_samples = swr_convert(swr_ctx_, resampled_data_, dst_nb_samples,
                               (const uint8_t **)src_data, src_nb_samples);
  if (nb_samples < 0) {
      std::cout << "swr_convert failed" << std::endl;
      return -1;
  }
  int dst_buffer_size = av_samples_get_buffer_size(
      &dst_linesize, dst_channels_, nb_samples, dst_sample_format_, 1);

  return av_audio_fifo_write(audioFifo_, (void **)resampled_data_, nb_samples);
}

bool AudioS16Resampler::receiveFrame(
    std::vector<std::shared_ptr<AVFrame>> &frames, int frame_size) {
  if (frame_size <= 0) {
    LOG_ERROR("frame_size is invalid");
    return false;
  }

  while (av_audio_fifo_size(audioFifo_) >= frame_size) {
    auto frame = readFrameFromFifo(frame_size);
    if (frame) {
      frames.push_back(frame);
    } else {
      LOG_ERROR("readFrameFromFifo failed");
      return false;
    }
  }

  return true;
}

std::shared_ptr<AVFrame>
AudioS16Resampler::readFrameFromFifo(const int frame_size) {
  auto frame = createFrameBySamples(frame_size);
  if (frame) {
    int ret = av_audio_fifo_read(audioFifo_, (void **)frame->data, frame_size);
    if (ret <= 0) {
      LOG_ERROR("av_audio_fifo_read failed");
    }
    frame->pts = cur_pts_;  // pts是用户自己定义的
    cur_pts_ += frame_size; // 为什么要往AV_NOPTS_VALUE上加？
    total_resampled_num_ += frame_size;
  }
  return frame;
}

std::shared_ptr<AVFrame>
AudioS16Resampler::createFrameBySamples(const int nb_samples) {
  auto doFreeFrame = [](AVFrame *p) {
    if (p)
      av_frame_free(&p);
  };
  auto frame = std::shared_ptr<AVFrame>(av_frame_alloc(), doFreeFrame);
  if (!frame) {
    std::cout << "av_frame_alloc frame failed " << std::endl;
    return {};
  }
  frame->nb_samples = nb_samples;
  frame->channel_layout = dst_channels_layout_;
  frame->format = dst_sample_format_;
  frame->sample_rate = dst_sample_rate_;
  int ret = av_frame_get_buffer(frame.get(), 0);
  if (ret < 0) {
    std::cout << "cannot allocate audio data buffer " << std::endl;
    return {};
  }
  return frame;
}

int AudioS16Resampler::resample(const uint8_t *in_data, int in_size,
                                uint8_t *out_data, int out_size) {
  // Perform the resampling
  return 0;
}

} // namespace Encoder