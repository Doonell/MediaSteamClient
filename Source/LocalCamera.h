#ifndef _LOCAL_CAMERA_H_
#define _LOCAL_CAMERA_H_

#include <memory>
#include <string>
#include <thread>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

class LocalCamera {
public:
  LocalCamera();
  ~LocalCamera();

  template <typename CallBack> void init(CallBack handleVideoPacket) {
    av_log_set_level(AV_LOG_INFO);
    avdevice_register_all();

    AVInputFormat *inputFmt = av_find_input_format("dshow");
    const char *deviceName = "video=Logi C270 HD WebCam"; // 设备名称

    AVFormatContext *inputCtx = nullptr;
    if (avformat_open_input(&inputCtx, deviceName, inputFmt, nullptr) != 0) {
      return -1;
    }

    if (avformat_find_stream_info(inputCtx, nullptr) < 0) {
      std::cerr << "未找到视频流\n";
      return -1;
    }

    int videoStreamIndex = -1;
    for (unsigned i = 0; i < inputCtx->nb_streams; ++i) {
      if (inputCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        videoStreamIndex = i;
        break;
      }
    }
    if (videoStreamIndex == -1) {
      std::cerr << "未找到视频流 videoStreamIndex: " << videoStreamIndex
                << "\n";
      return -1;
    }

    AVCodecParameters *codecPar = inputCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec *decoder = avcodec_find_decoder(codecPar->codec_id);
    AVCodecContext *decoderCtx = avcodec_alloc_context3(decoder);
    // decoderCtx.decoderCtx->pix_fmt == YUV or RGB
    avcodec_parameters_to_context(decoderCtx, codecPar);
    avcodec_open2(decoderCtx, decoder, nullptr);

    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    videoCodecCtx_ = avcodec_alloc_context3(encoder);
    videoCodecCtx_->height = decoderCtx->height;
    videoCodecCtx_->width = decoderCtx->width;
    videoCodecCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
    videoCodecCtx_->time_base = AVRational{1, 25};
    videoCodecCtx_->framerate = AVRational{25, 1};
    videoCodecCtx_->bit_rate = 400000;

    if (encoder->id == AV_CODEC_ID_H264) {
      av_opt_set(videoCodecCtx_->priv_data, "preset", "ultrafast", 0);
    }

    avcodec_open2(videoCodecCtx_, encoder, nullptr);

    FILE *outFile = fopen("output1.h264", "wb");

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    frame->format = videoCodecCtx_->pix_fmt;
    frame->width = videoCodecCtx_->width;
    frame->height = videoCodecCtx_->height;
    av_frame_get_buffer(frame, 32);

    // ==== MJPEG -> YUV420P ====
    SwsContext *swsCtx = sws_getContext(
        decoderCtx->width, decoderCtx->height, decoderCtx->pix_fmt,
        videoCodecCtx_->width, videoCodecCtx_->height, videoCodecCtx_->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    int frameCount = 0;
    while (frameCount < 100 * 30) {
      if (av_read_frame(inputCtx, packet) < 0)
        break;
      if (packet->stream_index != videoStreamIndex) {
        av_packet_unref(packet);
        continue;
      }

      avcodec_send_packet(decoderCtx, packet);
      while (avcodec_receive_frame(decoderCtx, frame) == 0) {
        AVFrame *swsFrame = av_frame_alloc();
        swsFrame->format = videoCodecCtx_->pix_fmt;
        swsFrame->width = videoCodecCtx_->width;
        swsFrame->height = videoCodecCtx_->height;
        av_frame_get_buffer(swsFrame, 32);

        sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
                  swsFrame->data, swsFrame->linesize);

        swsFrame->pts = frameCount++;

        avcodec_send_frame(videoCodecCtx_, swsFrame);
        AVPacket *encPkt = av_packet_alloc();
        while (avcodec_receive_packet(videoCodecCtx_, encPkt) == 0) {
          fwrite(encPkt->data, 1, encPkt->size, outFile);
          handleVideoPacket(*encPkt); // 处理视频包
          av_packet_unref(encPkt);
        }
        av_frame_free(&swsFrame);
      }

      av_packet_unref(packet);
    }

    fclose(outFile);
    av_frame_free(&frame);
    av_packet_free(&packet);
    sws_freeContext(swsCtx);
    avcodec_free_context(&decoderCtx);
    avcodec_free_context(&videoCodecCtx_);
    avformat_close_input(&inputCtx);

    return 0;
  };

  void start();
  void stop();

private:
  void captureFrame();
  void processFrame(AVFrame *frame);

  std::string deviceName_;
  AVFormatContext *formatCtx_;
  AVCodecContext *videoCodecCtx_;
  std::thread captureThread_;
  bool isCapturing_;
};

#endif // _LOCAL_CAMERA_H_
