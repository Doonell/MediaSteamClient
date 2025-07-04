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
#include <iostream>
#include <memory>
#include <cstdio>

using namespace TimeHelper;

int main() {
  std::cout << "Hello, World!" << std::endl;

  auto fileReader =
      Configuration::FileReaderFactory::createFileReader("config.json", "json");
  auto fileConfig = fileReader->parse();

  auto h264Encoder = std::make_shared<Encoder::H264Encoder>();
  if (h264Encoder->init() == false) {
      std::cout << "123123123" << std::endl;
      return false;
  }
  auto aacEncoder = std::make_shared<Encoder::AACEncoder>();
  aacEncoder->init();
  auto audioResampler = std::make_shared<Encoder::AudioS16Resampler>();
  audioResampler->init();

  double audio_frame_duration =
      1000.0 / aacEncoder->get_sample_rate() * aacEncoder->GetFrameSampleSize();
  LOG_INFO("audio_frame_duration:%lf", audio_frame_duration);
  AVPublishTime::GetInstance()->set_audio_frame_duration(audio_frame_duration);
  AVPublishTime::GetInstance()->set_audio_pts_strategy(
      AVPublishTime::PTS_RECTIFY);
  AVPublishTime::GetInstance()->set_video_pts_strategy(
      AVPublishTime::PTS_RECTIFY);

  auto &msgQueue =
      Middleware::MsgQueue<FLVMessage, FLVMetaMessage, VideoSequenceMessage,
                           H264RawMessage, AudioSpecificConfigMessage,
                           AudioRawDataMessage>::create();
  auto rtmpPusher =
      std::make_unique<TransProtocol::RTMPPusher<decltype(msgQueue)>>(
          msgQueue, aacEncoder, h264Encoder, audioResampler);

  auto yuvFileReader = std::make_shared<YUVFileReader>("test.yuv", 0, 0, 0, 0);
  yuvFileReader->start([&](uint8_t *data, int size) {
    rtmpPusher->sendVideoPacket(data, size);
  });

  auto pcmFileReader = std::make_shared<Reader::PCMFileReader>();
  pcmFileReader->start([&](uint8_t *data, int size) {
    rtmpPusher->sendAudioPacket(data, size);
  });
}
