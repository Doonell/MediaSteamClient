#include "H264Encoder.h"
#include "../Logger/Logger.h"
#include <cstdio>

namespace Encoder {

H264Encoder::H264Encoder(int width, int height, int fps, int bitrate, int gop,
                         int b_frames)
    : width_(width), height_(height), fps_(fps), bitrate_(bitrate), gop_(gop),
      b_frames_(b_frames) {}

bool H264Encoder::init() {
  std::cout << "libavcodec version:" << avcodec_version() << std::endl;
  int version = avcodec_version();
  int major = version >> 16;
  int minor = (version >> 8) & 0xFF;
  int micro = version & 0xFF;
  std::printf("libavcodec version: %d.%d.%d\n", major, minor, micro);
  print_all_encoders();
  // 寻找编码器
  codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (codec_ == NULL) {
    std::cout << "Can not find encoder!" << std::endl;
    return false;
  }

  count = 0;
  framecnt = 0;

  ctx_ = avcodec_alloc_context3(codec_);

  // Param that must set

  // 最大和最小量化系数，取值范围为0~51。
  ctx_->qmin = 10;
  ctx_->qmax = 31;

  // 编码后的视频帧大小，以像素为单位。
  ctx_->width = width_;
  ctx_->height = height_;

  // 编码后的码率：值越大越清晰，值越小越流畅。
  ctx_->bit_rate = bitrate_;

  // 每20帧插入一个I帧
  ctx_->gop_size = gop_;

  // 帧率的基本单位，time_base.num为时间线分子，time_base.den为时间线分母，帧率=分子/分母。
  ctx_->time_base.num = 1;
  ctx_->time_base.den = fps_;

  ctx_->framerate.num = fps_;
  ctx_->framerate.den = 1;

  // 图像色彩空间的格式，采用什么样的色彩空间来表明一个像素点。
  ctx_->pix_fmt = AV_PIX_FMT_YUV420P;

  // 编码器编码的数据类型
  ctx_->codec_type = AVMEDIA_TYPE_VIDEO;

  // Optional Param
  // 两个非B帧之间允许出现多少个B帧数，设置0表示不使用B帧，没有编码延时。B帧越多，压缩率越高。
  ctx_->max_b_frames = b_frames_;

  if (ctx_->codec_id == AV_CODEC_ID_H264) {
    av_dict_set(&param, "preset", "ultrafast", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);
  }
  if (ctx_->codec_id == AV_CODEC_ID_H265) {
    av_dict_set(&param, "preset", "ultrafast", 0);
    av_dict_set(&param, "tune", "zero-latency", 0);
  }

  ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // extradata拷贝 sps pps
  // 初始化视音频编码器的AVCodecContext
  if (avcodec_open2(ctx_, codec_, &param) < 0) {
    printf("Failed to open encoder! \n");
  }
  // 读取sps pps 信息
  if (ctx_->extradata) {
    LOG_INFO("extradata_size: " << ctx_->extradata_size);
    // 第一个为sps 7
    // 第二个为pps 8

    uint8_t *sps = ctx_->extradata + 4; // 直接跳到数据
    int sps_len = 0;
    uint8_t *pps = NULL;
    int pps_len = 0;
    uint8_t *data = ctx_->extradata + 4;
    for (int i = 0; i < ctx_->extradata_size - 4; ++i) {
      if (0 == data[i] && 0 == data[i + 1] && 0 == data[i + 2] &&
          1 == data[i + 3]) {
        pps = &data[i + 4];
        break;
      }
    }
    sps_len = int(pps - sps) - 4; // 4是00 00 00 01占用的字节
    pps_len = ctx_->extradata_size - 4 * 2 - sps_len;
    sps_.append(sps, sps + sps_len);
    pps_.append(pps, pps + pps_len);
  }

  // Init frame
  frame_ = av_frame_alloc();
  int pictureSize =
      avpicture_get_size(ctx_->pix_fmt, ctx_->width, ctx_->height);
  picture_buf_ = (uint8_t *)av_malloc(pictureSize);
  avpicture_fill((AVPicture *)frame_, picture_buf_, ctx_->pix_fmt, ctx_->width,
                 ctx_->height);

  frame_->width = ctx_->width;
  frame_->height = ctx_->height;
  frame_->format = ctx_->pix_fmt;

  // Init packet
  av_new_packet(&packet_, pictureSize);
  data_size_ = ctx_->width * ctx_->height;

  return 0;
}

H264Encoder::~H264Encoder() {
  if (ctx_)
    avcodec_close(ctx_);
  if (frame_)
    av_free(frame_);
  if (picture_buf_)
    av_free(picture_buf_);
  //av_free_packet(&packet_);
}

void H264Encoder::initByName()
{
    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        fprintf(stderr, "Could not find encoder: libx264\n");
    }
}

void H264Encoder::print_all_encoders() {
    const AVCodec* codec = nullptr;
    void* iter = nullptr;
    while ((codec = av_codec_iterate(&iter))) {
        if (av_codec_is_encoder(codec)) {
            printf("Encoder: %s\n", codec->name);
        }
    }
}

int H264Encoder::encode(uint8_t *in, uint32_t in_samples, uint8_t *out,
                        uint32_t &out_size) {
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
    LOG_ERROR("Failed to encode!");
    return -1;
  }

  if (got_picture == 1) {
    framecnt++;
    // 跳过00 00 00 01 startcode nalu
    memcpy(out, packet_.data + 4, packet_.size - 4);
    out_size = packet_.size - 4;
    av_packet_unref(&packet_); // 释放内存 不释放则内存泄漏
    return 0;
  }

  return -1;
}

} // namespace Encoder
