#ifndef _IAUDIO_ENCODER_H_
#define _IAUDIO_ENCODER_H_

#include <memory>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
}

class IAudioEncoder {
public:
  virtual ~IAudioEncoder() = default;

  virtual bool init() = 0;
  virtual int get_sample_rate() = 0;
  virtual int get_channels() = 0;
  virtual int get_profile() = 0;
  virtual uint32_t GetFrameSampleSize() = 0;
  virtual AVCodecContext *getCodecContext() = 0;
};

#endif // _IAUDIO_ENCODER_H_
