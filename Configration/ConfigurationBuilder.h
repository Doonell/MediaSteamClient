#include "IConfiguration.h"
#include <string>

namespace Configration {

class ConfigurationBuilder : public IConfigurationFacade {
public:
  ConfigurationBuilder() {}

  ConfigurationBuilder &set(const std::string &key, const std::string &value) {
    config_[key] = value;
    return *this;
  }

  std::string getRtmpUrl() { return config_["rtmpUrl"]; }
  int getAudioSampleRate() { return std::stoi(config_["audioSampleRate"]); }
  int getAudioBitrate() const { return std::stoi(config_["audioBitrate"]); }
  int getAudioChannels() const { return std::stoi(config_["audioChannels"]); }
  int getDesktopX() const { return std::stoi(config_["desktopX"]); }
  int getDesktopY() const { return std::stoi(config_["desktopY"]); }
  int getDesktopWidth() const { return std::stoi(config_["desktopWidth"]); }
  int getDesktopHeight() const { return std::stoi(config_["desktopHeight"]); }
  int getDesktopPixelFormat() const {
    return std::stoi(config_["desktopPixelFormat"]);
  }
  int getDesktopFps() const { return std::stoi(config_["desktopFps"]); }

  int getVideoWidth() const { return std::stoi(config_["videoWidth"]); }
  int getVideoHeight() const { return std::stoi(config_["videoHeight"]); }
  int getVideoFps() const { return std::stoi(config_["videoFps"]); }
  int getVideoGop() const { return std::stoi(config_["videoGop"]); }
  int getVideoBitrate() const { return std::stoi(config_["videoBitrate"]); }
  int getVideoBFrames() const { return std::stoi(config_["videoBFrames"]); }

  int getMicSampleRate() const { return std::stoi(config_["micSampleRate"]); }
  int getMicSampleFmt() const { return std::stoi(config_["micSampleFmt"]); }
  int getMicChannels() const { return std::stoi(config_["micChannels"]); }

private:
  std::map<std::string, std::string> config_;
};
} // namespace Configration