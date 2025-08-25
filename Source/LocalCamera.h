#ifndef _LOCAL_CAMERA_H_
#define _LOCAL_CAMERA_H_

#include <memory>
#include <string>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include "../Encoder/IVideoEncoder.h"
#include "../Util/TimeHelper.h"
using namespace TimeHelper;
namespace Encoder {

class LocalCamera : public IVideoEncoder {
public:
  LocalCamera() {}
  ~LocalCamera() {
    fclose(outFile_);
    sws_freeContext(swsCtx_);
    avcodec_free_context(&decoderCtx_);
    avcodec_free_context(&encCodecCtx_);
    avformat_close_input(&inputCtx_);
  }

  bool init() {
    av_log_set_level(AV_LOG_INFO);
    avdevice_register_all();

    AVInputFormat *inputFmt = av_find_input_format("dshow");
    const char *deviceName = "video=Logi C270 HD WebCam"; // �豸����

    AVDictionary *options = nullptr;
    av_dict_set(&options, "video_size", "1280x720", 0); // ���òɼ��ֱ���
    av_dict_set(&options, "framerate", "30", 0);        // ��ѡ������֡��
    if (avformat_open_input(&inputCtx_, deviceName, inputFmt, &options) != 0) {
      return false;
    }

    if (avformat_find_stream_info(inputCtx_, nullptr) < 0) {
      std::cerr << "δ�ҵ���Ƶ��\n";
      return false;
    }

    for (unsigned i = 0; i < inputCtx_->nb_streams; ++i) {
      if (inputCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        videoStreamIndex_ = i;
        break;
      }
    }
    if (videoStreamIndex_ == -1) {
      std::cerr << "δ�ҵ���Ƶ�� videoStreamIndex_: " << videoStreamIndex_
                << "\n";
      return false;
    }

    AVCodecParameters *codecPar =
        inputCtx_->streams[videoStreamIndex_]->codecpar;
    const AVCodec *decoder = avcodec_find_decoder(codecPar->codec_id);
    decoderCtx_ = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decoderCtx_, codecPar);
    avcodec_open2(decoderCtx_, decoder, nullptr);

    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    encCodecCtx_ = avcodec_alloc_context3(encoder);
    encCodecCtx_->height = decoderCtx_->height;
    encCodecCtx_->width = decoderCtx_->width;
    encCodecCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
    encCodecCtx_->time_base = AVRational{1, 30};

    if (encoder->id == AV_CODEC_ID_H264) {
      av_opt_set(encCodecCtx_->priv_data, "preset", "ultrafast", 0);
    }
    encCodecCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    avcodec_open2(encCodecCtx_, encoder, nullptr);
    // ��ȡsps pps ��Ϣ
    if (encCodecCtx_->extradata) {
      LOG_INFO("extradata_size: " << encCodecCtx_->extradata_size);
      // ��һ��Ϊsps 7
      // �ڶ���Ϊpps 8
      uint8_t *sps = encCodecCtx_->extradata + 4; // ֱ����������
      int sps_len = 0;
      uint8_t *pps = NULL;
      int pps_len = 0;
      uint8_t *data = encCodecCtx_->extradata + 4;
      for (int i = 0; i < encCodecCtx_->extradata_size - 4; ++i) {
        if (0 == data[i] && 0 == data[i + 1] && 0 == data[i + 2] &&
            1 == data[i + 3]) {
          pps = &data[i + 4];
          break;
        }
      }
      sps_len = int(pps - sps) - 4; // 4��00 00 00 01ռ�õ��ֽ�
      pps_len = encCodecCtx_->extradata_size - 4 * 2 - sps_len;
      sps_.append(sps, sps + sps_len);
      pps_.append(pps, pps + pps_len);
    }

    // ==== MJPEG -> YUV420P ====
    swsCtx_ = sws_getContext(decoderCtx_->width, decoderCtx_->height,
                             decoderCtx_->pix_fmt, encCodecCtx_->width,
                             encCodecCtx_->height, encCodecCtx_->pix_fmt,
                             SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (swsCtx_ == nullptr) {
      return false;
    }

    return true;
  }

  void start(const std::function<void(AVPacket &)> &handleVideoPacket) {
    outFile_ = fopen("output1.h264", "wb");

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    frame->format = encCodecCtx_->pix_fmt;
    frame->width = encCodecCtx_->width;
    frame->height = encCodecCtx_->height;
    av_frame_get_buffer(frame, 32);

    int frameCount = 0;
    while (1) {
      AVPublishTime::getInstance()->reset();
      if (av_read_frame(inputCtx_, packet) < 0)
        break;
      if (packet->stream_index != videoStreamIndex_) {
        av_packet_unref(packet);
        continue;
      }
      std::cout << "Current interleaved Time from  input: "
                << AVPublishTime::getInstance()->getCurrenTime() << std::endl;

      avcodec_send_packet(decoderCtx_, packet);
      while (avcodec_receive_frame(decoderCtx_, frame) == 0) {
        AVFrame *swsFrame = av_frame_alloc();
        swsFrame->format = encCodecCtx_->pix_fmt;
        swsFrame->width = encCodecCtx_->width;
        swsFrame->height = encCodecCtx_->height;
        av_frame_get_buffer(swsFrame, 32);

        sws_scale(swsCtx_, frame->data, frame->linesize, 0, frame->height,
                  swsFrame->data, swsFrame->linesize);

        swsFrame->pts = frameCount++;

        avcodec_send_frame(encCodecCtx_, swsFrame);
        AVPacket *encPkt = av_packet_alloc();
        while (avcodec_receive_packet(encCodecCtx_, encPkt) == 0) {
          fwrite(encPkt->data, 1, encPkt->size, outFile_);
          handleVideoPacket(*encPkt); // ������Ƶ��
          av_packet_unref(encPkt);
        }
        av_frame_free(&swsFrame);
      }
      av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    return;
  }

  int get_width() { return encCodecCtx_ ? encCodecCtx_->width : 0; }
  int get_height() { return encCodecCtx_ ? encCodecCtx_->height : 0; }
  double get_framerate() {
    return encCodecCtx_->framerate.num / encCodecCtx_->framerate.den;
  }
  int64_t get_bit_rate() { return encCodecCtx_ ? encCodecCtx_->bit_rate : 0; }
  uint8_t *get_sps_data() { return (uint8_t *)(sps_.c_str()); }
  int get_sps_size() { return sps_.size(); }
  inline uint8_t *get_pps_data() { return (uint8_t *)(pps_.c_str()); }
  int get_pps_size() { return pps_.size(); }
  AVCodecContext *getCodecContext() { return encCodecCtx_; }

private:
  std::string deviceName_;
  AVFormatContext *formatCtx_;
  AVCodecContext *encCodecCtx_;
  std::thread captureThread_;
  bool isCapturing_;
  std::string sps_;
  std::string pps_;
  FILE *outFile_;
  SwsContext *swsCtx_ = nullptr;
  AVCodecContext *decoderCtx_ = nullptr;
  AVFormatContext *inputCtx_ = nullptr;
  int videoStreamIndex_ = -1;
};
} // namespace Encoder
#endif // _LOCAL_CAMERA_H_
