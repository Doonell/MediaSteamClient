#include "AACEncoder.h"
#include "../Logger/Logger.h"

namespace Encoder {

AACEncoder::AACEncoder(int sample_rate, int channels, int bitrate,
                       int channel_layout)
    : sample_rate_(sample_rate), channels_(channels), bitrate_(bitrate),
      channel_layout_(channel_layout) {}

AACEncoder::~AACEncoder() {
  if (ctx_) {
    avcodec_free_context(&ctx_);
  }
  if (frame_) {
    av_frame_free(&frame_);
  }
}

bool AACEncoder::init() {
  // locate the encoder
  codec_ = avcodec_find_encoder(AV_CODEC_ID_AAC);
  if (!codec_) {
    LOG_ERROR("Codec not found Codec required");
    return false;
  }

  // init codec as configured
  ctx_ = avcodec_alloc_context3(codec_);
  if (!ctx_) {
    LOG_ERROR("Could not allocate audio codec context");
    return false;
  }

  ctx_->channels = channels_;
  ctx_->channel_layout = channel_layout_;
  ctx_->sample_rate = sample_rate_;
  ctx_->sample_fmt = AV_SAMPLE_FMT_FLTP;
  ctx_->bit_rate = bitrate_;
  ctx_->thread_count = 1;
  ctx_->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

  if (avcodec_open2(ctx_, codec_, nullptr) < 0) {
    LOG_ERROR("Could not open codec");
    return false;
  }

  frame_ = av_frame_alloc();
  if (!frame_) {
    LOG_ERROR("Could not allocate audio frame");
    return false;
  }

  frame_->nb_samples = ctx_->frame_size;
  frame_->format = ctx_->sample_fmt;
  frame_->channel_layout = ctx_->channel_layout;
  frame_->channels = ctx_->channels;
  av_frame_get_buffer(frame_, 0);
  return true;
}

int32_t AACEncoder::encode(AVFrame *frame, uint8_t *out, int out_len) {
  int got_output = 0;

  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = out;
  pkt.size = out_len;

  if (avcodec_encode_audio2(ctx_, &pkt, frame, &got_output) < 0) {
    LOG_ERROR("Error encoding audio");
    return -1;
  }

  if (!got_output) {
    LOG_ERROR("AAC: could not get output packet");
    return -1;
  }

  return pkt.size;
}
} // namespace Encoder