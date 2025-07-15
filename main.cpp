#include "Configuration/FileReaderFactory.h"
#include "Configuration/IConfigurationFacade.h"
#include "Encoder/AACEncoder.h"
#include "Encoder/H264Encoder.h"
#include "Logger/Logger.h"
#include "Middleware/MsgQueue.h"
#include "Source/PCMFileReader.h"
#include "Source/YUVFileReader.h"
#include "TransProtocol/RTMPPusher.h"
#include "Util/TimeHelper.h"
#include <cstdio>
#include <iostream>
#include <memory>

using namespace TimeHelper;

int main() {
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
  LOG_INFO("audio_frame_duration:%lf", audio_frame_duration);
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
  //auto yuvFileReader =
  //    std::make_shared<YUVFileReader>("720x480_25fps_420p.yuv");
  //yuvFileReader->init();
  //yuvFileReader->start(
  //    [&](uint8_t *yuv, int size) { rtmpPusher->sendVideoPacket(yuv, size); });

  auto pcmFileReader = std::make_shared<Reader::PCMFileReader>();
  pcmFileReader->init();
  pcmFileReader->start(
      [&](uint8_t *pcm, int size) { rtmpPusher->sendAudioPacket(pcm, size); });

  while (1) {
    ::sleep(1);
  }
}
