#ifndef _CONFIGURATION_ICONFIGURATIONFACADE_H_
#define _CONFIGURATION_ICONFIGURATIONFACADE_H_

#include <string>

namespace Configuration {

class IConfigurationFacade {
public:
  virtual std::string getRtmpUrl() = 0;

  virtual int getAudioSampleRate() = 0;
  virtual int getAudioBitrate() = 0;
  virtual int getAudioChannels() = 0;

  virtual int getDesktopX() = 0;
  virtual int getDesktopY() = 0;
  virtual int getDesktopWidth() = 0;
  virtual int getDesktopHeight() = 0;
  virtual int getDesktopPixelFormat() = 0;
  virtual int getDesktopFps() = 0;

  //  ”∆µ±‡¬Î Ù–‘
  virtual int getVideoWidth() = 0;
  virtual int getVideoHeight() = 0;
  virtual int getVideoFps() = 0;
  virtual int getVideoGop() = 0;
  virtual int getVideoBitrate() = 0;
  virtual int getVideoBFrames() = 0;

  virtual int getMicSampleRate() = 0;
  virtual int getMicSampleFmt() = 0;
  virtual int getMicChannels() = 0;
};
} // namespace Configuration

#endif // _CONFIGURATION_ICONFIGURATIONFACADE_H_
