#include "Configuration/FileReaderFactory.h"
#include "Configuration/IConfigurationFacade.h"
#include "Encoder/AACEncoder.h"
#include "Encoder/H264Encoder.h"
#include "Logger/Logger.h"
#include "Middleware/MsgQueue.h"
#include "Source/PCMFileReader.h"
#include "Source/YUVFileReader.h"
#include "TransProtocol/RTMPPuller.h"
#include "TransProtocol/RTMPPusher.h"
#include "Util/TimeHelper.h"
#include <cstdio>
#include <iostream>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

using namespace TimeHelper;

int printLocalDevicesList() {
  av_log_set_level(AV_LOG_INFO);
  avdevice_register_all();
  // ==== ?????????υτ???? Windows DirectShow ????? ====
  AVInputFormat *inputFmt = av_find_input_format("dshow");

  // ????????????????????υτ
  AVDictionary *options = nullptr;
  av_dict_set(&options, "list_devices", "true", 0);

  AVFormatContext *inputCtx = nullptr;
  if (avformat_open_input(&inputCtx, "video=dummy", inputFmt, &options) != 0) {
    std::cerr << "??????????υτ\n";
    return -1;
  }
}

int pushLocalStream() {
  av_log_set_level(AV_LOG_INFO);
  avdevice_register_all();
  // ==== ?????????υτ???? Windows DirectShow ????? ====
  AVInputFormat *inputFmt = av_find_input_format("dshow");
  const char *deviceName = "video=Logi C270 HD WebCam"; // ?I?????????????

  AVFormatContext *inputCtx = nullptr;
  if (avformat_open_input(&inputCtx, deviceName, inputFmt, nullptr) != 0) {
    std::cerr << "??????????υτ\n";
    return -1;
  }

  if (avformat_find_stream_info(inputCtx, nullptr) < 0) {
    std::cerr << "???????????????\n";
    return -1;
  }

  // ==== ???????? ====
  int videoStreamIndex = -1;
  for (unsigned i = 0; i < inputCtx->nb_streams; ++i) {
    if (inputCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStreamIndex = i;
      break;
    }
  }
  if (videoStreamIndex == -1) {
    std::cerr << "??????????\n";
    return -1;
  }

  // ==== ??????????? ====
  AVCodecParameters *codecPar = inputCtx->streams[videoStreamIndex]->codecpar;
  const AVCodec *decoder = avcodec_find_decoder(codecPar->codec_id);
  AVCodecContext *decoderCtx = avcodec_alloc_context3(decoder);
  // decoderCtx???decoderCtx->pix_fmt == YUV????RGB
  avcodec_parameters_to_context(decoderCtx, codecPar);
  avcodec_open2(decoderCtx, decoder, nullptr);

  // ==== ?????????????H.264??====
  const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
  AVCodecContext *encoderCtx = avcodec_alloc_context3(encoder);
  encoderCtx->height = decoderCtx->height;
  encoderCtx->width = decoderCtx->width;
  encoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
  encoderCtx->time_base = AVRational{1, 25};
  encoderCtx->framerate = AVRational{25, 1};
  encoderCtx->bit_rate = 400000;

  if (encoder->id == AV_CODEC_ID_H264) {
    av_opt_set(encoderCtx->priv_data, "preset", "ultrafast", 0);
  }

  avcodec_open2(encoderCtx, encoder, nullptr);

  // ==== ???????? ====
  FILE *outFile = fopen("output1.h264", "wb");

  AVPacket *packet = av_packet_alloc();
  AVFrame *frame = av_frame_alloc();
  frame->format = encoderCtx->pix_fmt;
  frame->width = encoderCtx->width;
  frame->height = encoderCtx->height;
  av_frame_get_buffer(frame, 32);

  // ==== ???????????? MJPEG -> YUV420P??====
  SwsContext *swsCtx =
      sws_getContext(decoderCtx->width, decoderCtx->height, decoderCtx->pix_fmt,
                     encoderCtx->width, encoderCtx->height, encoderCtx->pix_fmt,
                     SWS_BILINEAR, nullptr, nullptr, nullptr);

  int frameCount = 0;
  while (frameCount < 100 * 30) { // ??? 100 ?
    if (av_read_frame(inputCtx, packet) < 0)
      break;
    if (packet->stream_index != videoStreamIndex) {
      av_packet_unref(packet);
      continue;
    }

    // ????
    avcodec_send_packet(decoderCtx, packet);
    while (avcodec_receive_frame(decoderCtx, frame) == 0) {
      // ??????
      AVFrame *swsFrame = av_frame_alloc();
      swsFrame->format = encoderCtx->pix_fmt;
      swsFrame->width = encoderCtx->width;
      swsFrame->height = encoderCtx->height;
      av_frame_get_buffer(swsFrame, 32);

      sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
                swsFrame->data, swsFrame->linesize);

      swsFrame->pts = frameCount++;

      // ????
      avcodec_send_frame(encoderCtx, swsFrame);
      AVPacket *encPkt = av_packet_alloc();
      while (avcodec_receive_packet(encoderCtx, encPkt) == 0) {
        fwrite(encPkt->data, 1, encPkt->size, outFile);
        av_packet_unref(encPkt);
      }
      av_packet_free(&encPkt);
      av_frame_free(&swsFrame);
    }

    av_packet_unref(packet);
  }

  // ==== ???? ====
  fclose(outFile);
  av_frame_free(&frame);
  av_packet_free(&packet);
  sws_freeContext(swsCtx);
  avcodec_free_context(&decoderCtx);
  avcodec_free_context(&encoderCtx);
  avformat_close_input(&inputCtx);

  std::cout << "??????\n";
  return 0;
}

int main() {
  auto rtmppuller = std::make_shared<TransProtocol::RTMPPuller>();
  rtmppuller->start();
  for (;;) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  exit(1);
  std::cout << "Hello, World!" << std::endl;
  auto fileReader =
      Configuration::FileReaderFactory::createFileReader("config.json", "json");
  auto fileConfig = fileReader->parse();

  auto h264Encoder = std::make_shared<Encoder::H264Encoder>();
  if (!h264Encoder->init()) {
    std::cout << "H264Encoder init failed" << std::endl;
    return -1;
  }
  auto aacEncoder = std::make_shared<Encoder::AACEncoder>();
  if (!aacEncoder->init()) {
    std::cout << "AACEncoder init failed" << std::endl;
    return -1;
  }
  auto audioResampler = std::make_shared<Encoder::AudioS16Resampler>();
  if (!audioResampler->init()) {
    std::cout << "AudioS16Resampler init failed" << std::endl;
    return -1;
  }

  double audio_frame_duration =
      1000.0 / aacEncoder->get_sample_rate() * aacEncoder->GetFrameSampleSize();

  AVPublishTime::GetInstance()->set_audio_frame_duration(audio_frame_duration);
  AVPublishTime::GetInstance()->set_audio_pts_strategy(
      AVPublishTime::PTS_RECTIFY);
  AVPublishTime::GetInstance()->set_video_pts_strategy(
      AVPublishTime::PTS_RECTIFY);

  auto &msgQueue = Middleware::MsgQueue<
      FLVAudioMessage, FLVMetaMessage, VideoSequenceMessage, H264RawMessage,
      AudioSpecificConfigMessage, AudioRawDataMessage>::create();
  auto rtmpPusher =
      std::make_shared<TransProtocol::RTMPPusher<decltype(msgQueue)>>(
          msgQueue, aacEncoder, h264Encoder, audioResampler);
  rtmpPusher->start();

  auto yuvFileReader =
      std::make_shared<YUVFileReader>("720x480_25fps_420p.yuv");
  yuvFileReader->init();
  std::thread yuvThread([&]() {
    yuvFileReader->start([&](uint8_t *yuv, int size) {
      rtmpPusher->sendVideoPacket(yuv, size);
    });
  });
  yuvThread.detach();

  auto pcmFileReader = std::make_shared<Reader::PCMFileReader>();
  pcmFileReader->init();

  std::thread pcmThread([&]() {
    pcmFileReader->start([&](uint8_t *pcm, int size) {
      rtmpPusher->sendAudioPacket(pcm, size);
    });
  });
  pcmThread.detach();

  while (1) {
    ::sleep(1);
  }
}
