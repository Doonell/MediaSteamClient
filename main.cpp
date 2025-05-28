#include "Configuration/FileReaderFactory.h"
#include "Configuration/IConfiguration.h"
#include <iostream>

int main() {
  std::cout << "Hello, World!" << std::endl;

  FileReaderFactory fileReaderFactory;
  std::shared_ptr<IReader> fileReader =
      fileReaderFactory.createFileReader("config.json", "json");
  std::shared_ptr<IConfigurationFacade> fileConfig = fileReader->parse();
  // std::shared_ptr<IProtocolEntity> rtmpEntity =
  // std::shared_ptr<RTMP>(fileConfig); std::unique_ptr<AudioEnCoder>
  // audioEncoder = std::make_unique<AudioEnCoder>(fileConfig, rtmpEntity,
  // inputPCMReader); std::unique_ptr<AudioEnCoder> videoEncoder =
  // std::make_unique<VideoEncoder>(fileConfig, rtmpEntity, inputYUVFileReader);
}
