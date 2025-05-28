#incldue < string>

namespace configuration {
class IConfigurationFacade {
public:
  virtual ~IConfigurationFacade() = default;

  virtual std::string getRtmpUrl() const = 0;

  virtual int getAudioSampleRate() const = 0;
  virtual int getAudioBitrate() const = 0;
  virtual int getAudioChannels() const = 0;

  virtual int getDesktopX() const = 0;
  virtual int getDesktopY() const = 0;
  virtual int getDesktopWidth() const = 0;
  virtual int getDesktopHeight() const = 0;
  virtual int getDesktopPixeFormat() const = 0;
  virtual int getDesktopFps() const = 0;

  // 视频编码属性
  virtual int getVideoWidth() const = 0;
  virtual int getVideoHeight() const = 0;
  virtual int getVideoFps() const = 0;
  virtual int getVideoGop() const = 0;
  virtual int getVideoBitrate() const = 0;
  virtual int getVideoBFrames() const = 0;

  virtual int getMicSampleRate() const = 0;
  virtual int getMicSampleFmt() const = 0;
  virtual int getMicChannels() const = 0;
};
} // namespace configuration