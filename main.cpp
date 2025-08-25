#include "Configuration/FileReaderFactory.h"
#include "Configuration/IConfigurationFacade.h"
#include "Encoder/AACEncoder.h"
#include "Encoder/H264Encoder.h"
#include "Logger/Logger.h"
#include "Middleware/MsgQueue.h"
#include "Source/LocalCamera.h"
#include "Source/PCMFileReader.h"
#include "Source/YUVFileReader.h"
#include "TransProtocol/RTMPPuller.h"
#include "TransProtocol/RTMPPusher.h"
#include "TransProtocol/RTSPPusher.h"
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

int main() {
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

  auto localCamera = std::make_shared<Encoder::LocalCamera>();
  if (!localCamera->init()) {
    std::cout << "LocalCamera init failed" << std::endl;
    return -1;
  }

  double audio_frame_duration =
      1000.0 / aacEncoder->get_sample_rate() * aacEncoder->GetFrameSampleSize();

  AVPublishTime::getInstance()->set_audio_frame_duration(audio_frame_duration);

  auto &msgQueue = MessagesQueue::create();

  auto rtspPusher =
      std::make_shared<TransProtocol::RTSPPusher<decltype(msgQueue)>>(
          msgQueue, nullptr, localCamera, nullptr);
  rtspPusher->start();

  localCamera->start(
      [&](AVPacket &packet) { rtspPusher->sendRawVideoPacket(packet); });
#if 0
   pushLocalStream(
       [&](AVPacket &packet) { rtspPusher->sendRawVideoPacket(packet); });
   auto rtmpPusher =
       std::make_shared<TransProtocol::RTMPPusher<decltype(msgQueue)>>(
           msgQueue, aacEncoder, h264Encoder, audioResampler);
   rtmpPusher->start();

   auto yuvFileReader =
       std::make_shared<YUVFileReader>("720x480_25fps_420p.yuv");
   yuvFileReader->init();
   std::thread yuvThread([&]() {
     yuvFileReader->start([&](uint8_t *yuv, int size) {
       rtspPusher->sendVideoPacket(yuv, size);
     });
   });
   yuvThread.detach();

   auto pcmFileReader = std::make_shared<Reader::PCMFileReader>();
   pcmFileReader->init();

   std::thread pcmThread([&]() {
     pcmFileReader->start([&](uint8_t *pcm, int size) {
       rtspPusher->sendAudioPacket(pcm, size);
     });
   });
   pcmThread.detach();
#endif 
  while (1) {
    ::sleep(1);
  }
}
