#ifndef _CONFIGURATION_CONFIGURATION_BUILDER_H_
#define _CONFIGURATION_CONFIGURATION_BUILDER_H_

#include "IConfigurationFacade.h"
#include <map>
#include <string>

namespace Configuration {

class ConfigurationBuilder : public IConfigurationFacade {
public:
  ConfigurationBuilder() = default;
  ~ConfigurationBuilder() = default;

  ConfigurationBuilder &set(const std::string &key, const std::string &value) {
    config_[key] = value;
    return *this;
  }

  std::string getRtmpUrl() { return config_["rtmpUrl"]; }
  int getAudioSampleRate() { return std::stoi(config_["audioSampleRate"]); }
  int getAudioBitrate() { return std::stoi(config_["audioBitrate"]); }
  int getAudioChannels() { return std::stoi(config_["audioChannels"]); }
  int getDesktopX() { return std::stoi(config_["desktopX"]); }
  int getDesktopY() { return std::stoi(config_["desktopY"]); }
  int getDesktopWidth() { return std::stoi(config_["desktopWidth"]); }
  int getDesktopHeight() { return std::stoi(config_["desktopHeight"]); }
  int getDesktopPixelFormat() {
    return std::stoi(config_["desktopPixelFormat"]);
  }
  int getDesktopFps() { return std::stoi(config_["desktopFps"]); }

  int getVideoWidth() { return std::stoi(config_["videoWidth"]); }
  int getVideoHeight() { return std::stoi(config_["videoHeight"]); }
  int getVideoFps() { return std::stoi(config_["videoFps"]); }
  int getVideoGop() { return std::stoi(config_["videoGop"]); }
  int getVideoBitrate() { return std::stoi(config_["videoBitrate"]); }
  int getVideoBFrames() { return std::stoi(config_["videoBFrames"]); }

  int getMicSampleRate() { return std::stoi(config_["micSampleRate"]); }
  int getMicSampleFmt() { return std::stoi(config_["micSampleFmt"]); }
  int getMicChannels() { return std::stoi(config_["micChannels"]); }

private:
  std::map<std::string, std::string> config_;
};
} // namespace Configuration
#endif // _CONFIGURATION_CONFIGURATION_BUILDER_H_