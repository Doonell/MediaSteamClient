#include "AudioS16Resampler.h"

namespace Encoder {

AudioS16Resampler::AudioS16Resampler(int srcChannels, int dstChannels,
                                     int srcSampleRate, int dstSampleRate,
                                     int srcSampleFormat, int dstSampleFormat)
    : src_channels_(srcChannels), dst_channels_(dstChannels),
      src_sample_rate_(srcSampleRate), dst_sample_rate_(dstSampleRate),
      src_sample_format_(srcSampleFormat), dst_sample_format_(dstSampleFormat) {
  src_channels_layout_ = av_get_default_channel_layout(srcChannels);
  dst_channels_layout_ = av_get_default_channel_layout(dstChannels);
}

bool AudioS16Resampler::init() {
  // Initialize the resampler
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
      sw_opt_set_sample_fmt(swr_ctx_, "in_sample_fmt", src_sample_format_, 0);

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

  dst_nb_samples = av_rescale_rnd(src_nb_samples_, dst_sample_rate_,
                                  src_sample_rate_, AV_ROUND_UP);
  max_dst_nb_samples_ = dst_nb_samples;
  int ret = av_samples_alloc_array_and_samples(&resampled_data_, &dst_channels_,
                                               &dst_sample_rate_,
                                               &dst_sample_format_, 0);
  if (ret < 0) {
    LOG_ERROR("Could not allocate resampled data");
    return false;
  }
  return true;
}

int AudioResampler::sendResampleFrame(uint8_t *in_pcm, const int in_size) {
  auto frame = std::shared_ptr<AVFrame>(
      av_frame_alloc(), [](AVFrame *frame) { av_frame_free(&frame); });
  if (!frame) {
    LOG_ERROR("Could not allocate audio frame");
    return -1;
  }

  if (start_pts_ == AV_NOPTS_VALUE && frame->pts != AV_NOPTS_VALUE) {
    start_pts_ = frame->pts;
    cur_pts_ = frame->pts;
  }

  int src_nb_samples = frame->nb_samples;
  uint8_t **src_data = frame->extended_data;

  int delay = swr_get_delay(swr_ctx_, src_sample_rate_);
  int dst_nb_samples = av_rescale_rnd(delay + src_nb_samples, dst_sample_rate_,
                                      src_sample_rate_, AV_ROUND_UP);
  if (dst_nb_samples > max_dst_nb_samples_) {
    LOG_ERROR("dst_nb_samples is too large");
    return -1;
  }

  int nb_samples = swr_convert(swr_ctx_, resampled_data_, dst_nb_samples,
                               (const uint8_t **)src_data, src_nb_samples);
  int dst_buffer_size = av_samples_get_buffer_size(
      &dst_linesize, dst_channels_, nb_samples, dst_sample_format_, 1);

  audioFifo_ = av_audio_fifo_alloc(dst_sample_format_, dst_channels_, 1);
  if (!audioFifo_) {
    LOG_ERROR("Could not allocate audio FIFO");
    return false;
  }
  return av_audio_fifo_write(audioFifo_, (void **)resampled_data_, nb_samples);
}

bool AudioS16Resampler::receiveResampledFrame(
    vector<shared_ptr<AVFrame>> &frames, int desired_size) {
  if (desire_size <= 0) {
    LOG_ERROR("desired_size is invalid");
    return false;
  }

  while (av_audio_fifo_size(audioFifo_) >= desired_size) {
    auto frame = readFrameFromFifo(desired_size);
    if (frame) {
      frames.push_back(frame);
    } else {
      LOG_ERROR("readFrameFromFifo failed");
      return false;
    }
  }

  return true;
}

shared_ptr<AVFrame>
AudioS16Resampler::readFrameFromFifo(const int desiredSize) {
  auto frame = createFrameBySamples(desiredSize);
  if (frame) {
    int ret =
        av_audio_fifo_read(audio_fifo_, (void **)frame->data, desiredSize);
    if (ret <= 0) {
      LOG_ERROR("av_audio_fifo_read failed");
    }
    frame->pts = cur_pts_;   // pts是用户自己定义的
    cur_pts_ += desiredSize; // 为什么要往AV_NOPTS_VALUE上加？
    total_resampled_num_ += desiredSize;
  }
  return frame;
}

shared_ptr<AVFrame>
AudioS16Resampler::createFrameBySamples(const int nb_samples) {
  auto doFreeFrame = [](AVFrame *p) {
    if (p)
      av_frame_free(&p);
  };
  auto frame = shared_ptr<AVFrame>(av_frame_alloc(), doFreeFrame);
  if (!frame) {
    LOG_ERROR("%s av_frame_alloc frame failed", logtag_.c_str());
    return {};
  }
  frame->nb_samples = nb_samples;
  frame->channel_layout = dstChannelsLayout_;
  frame->format = dstSampleFormat_;
  frame->sample_rate = dstSampleRate_;
  int ret = av_frame_get_buffer(frame.get(), 0);
  if (ret < 0) {
    LOG_INFO("%s cannot allocate audio data buffer", logtag_.c_str());
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