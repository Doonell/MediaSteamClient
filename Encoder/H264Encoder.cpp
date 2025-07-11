#include "H264Encoder.h"
#include "../Logger/Logger.h"
#include <cstdio>

namespace Encoder {

H264Encoder::H264Encoder(int width, int height, int fps, int bitrate, int gop,
                         int b_frames)
    : width_(width), height_(height), fps_(fps), bitrate_(bitrate), gop_(gop),
      b_frames_(b_frames) {
  codec_ = nullptr;
}

bool H264Encoder::init() {
  std::cout << "libavcodec version:" << avcodec_version() << std::endl;
  int version = avcodec_version();
  int major = version >> 16;
  int minor = (version >> 8) & 0xFF;
  int micro = version & 0xFF;
  std::printf("libavcodec version: %d.%d.%d\n", major, minor, micro);

  // Ѱ�ұ�����
  codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (codec_ == nullptr) {
    std::cout << "Can not find encoder!" << std::endl;
    return false;
  }

  count = 0;
  framecnt = 0;

  ctx_ = avcodec_alloc_context3(codec_);

  // Param that must set

  // ������С����ϵ����ȡֵ��ΧΪ0~51��
  ctx_->qmin = 10;
  ctx_->qmax = 31;

  // ��������Ƶ֡��С��������Ϊ��λ��
  ctx_->width = width_;
  ctx_->height = height_;

  // ���������ʣ�ֵԽ��Խ������ֵԽСԽ������
  ctx_->bit_rate = bitrate_;

  // ÿ20֡����һ��I֡
  ctx_->gop_size = gop_;

  // ֡�ʵĻ�����λ��time_base.numΪʱ���߷��ӣ�time_base.denΪʱ���߷�ĸ��֡��=����/��ĸ��
  ctx_->time_base.num = 1;
  ctx_->time_base.den = fps_;

  ctx_->framerate.num = fps_;
  ctx_->framerate.den = 1;

  // ͼ��ɫ�ʿռ�ĸ�ʽ������ʲô����ɫ�ʿռ�������һ�����ص㡣
  ctx_->pix_fmt = AV_PIX_FMT_YUV420P;

  // �������������������
  ctx_->codec_type = AVMEDIA_TYPE_VIDEO;

  // Optional Param
  // ������B֮֡��������ֶ��ٸ�B֡��������0��ʾ��ʹ��B֡��û�б�����ʱ��B֡Խ�࣬ѹ����Խ�ߡ�
  ctx_->max_b_frames = b_frames_;

  if (ctx_->codec_id == AV_CODEC_ID_H264) {
    av_dict_set(&param, "preset", "ultrafast", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);
  }
  if (ctx_->codec_id == AV_CODEC_ID_H265) {
    av_dict_set(&param, "preset", "ultrafast", 0);
    av_dict_set(&param, "tune", "zero-latency", 0);
  }

  ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // extradata���� sps pps
  // ��ʼ������Ƶ��������AVCodecContext
  if (avcodec_open2(ctx_, codec_, &param) < 0) {
    printf("Failed to open encoder! \n");
  }
  // ��ȡsps pps ��Ϣ
  if (ctx_->extradata) {
    LOG_INFO("extradata_size: " << ctx_->extradata_size);
    // ��һ��Ϊsps 7
    // �ڶ���Ϊpps 8

    uint8_t *sps = ctx_->extradata + 4; // ֱ����������
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
    sps_len = int(pps - sps) - 4; // 4��00 00 00 01ռ�õ��ֽ�
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

  return true;
}

H264Encoder::~H264Encoder() {
  if (ctx_)
    avcodec_close(ctx_);
  if (frame_)
    av_free(frame_);
  if (picture_buf_)
    av_free(picture_buf_);
  av_free_packet(&packet_);
}

const AVCodec *
H264Encoder::find_encoder_by_name(const std::string &codec_name) {
  const AVCodec *codec = nullptr;
  void *iter = nullptr;
  while ((codec = av_codec_iterate(&iter))) {
    if (av_codec_is_encoder(codec) && codec_name == codec->name) {
      printf("Encoder: %s\n", codec->name);
      return codec;
    }
  }
}

// template <typename H264Callback>
// int H264Encoder::encode(uint8_t *in, H264Callback handleH264) {
//   int out_size = 0;
//   std::shared_ptr<uint8_t[]> out(new uint8_t[VIDEO_NALU_BUF_MAX_SIZE]);

//   frame_->data[0] = in;                      // Y
//   frame_->data[1] = in + data_size_;         // U
//   frame_->data[2] = in + data_size_ * 5 / 4; // V
//   frame_->pts = (count++) * (ctx_->time_base.den) /
//                 ((ctx_->time_base.num) * 25); // ʱ���
//   av_init_packet(&packet_);
//   // Encode
//   int got_picture = 0;
//   int ret = avcodec_encode_video2(ctx_, &packet_, frame_, &got_picture);

//   if (ret < 0) {
//     LOG_ERROR("Failed to encode!");
//     return -1;
//   }

//   if (got_picture == 1) {
//     framecnt++;
//     // ����00 00 00 01 startcode nalu
//     memcpy(out.get(), packet_.data + 4, packet_.size - 4);
//     out_size = packet_.size - 4;
//     handleH264(out, out_size);
//     av_packet_unref(&packet_); // �ͷ��ڴ� ���ͷ����ڴ�й©
//     return 0;
//   }
//   std::cout << "Got picture is not 1, got_picture: " << got_picture
//             << std::endl;

//   return -1;
// }

} // namespace Encoder
