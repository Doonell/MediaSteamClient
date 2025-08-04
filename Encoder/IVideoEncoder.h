#ifndef _IVIDEO_ENCODER_H_
#define _IVIDEO_ENCODER_H_

#include <functional>
#include <memory>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
}

class IVideoEncoder {
public:
  virtual ~IVideoEncoder() = default;

  virtual bool init() = 0;
  virtual void encode(uint8_t *, std::function<void(AVPacket &)>) = 0;
  virtual int get_width() = 0;
  virtual int get_height() = 0;
  virtual double get_framerate() = 0;
  virtual int64_t get_bit_rate() = 0;
  virtual uint8_t *get_sps_data() = 0;
  virtual int get_sps_size() = 0;
  virtual uint8_t *get_pps_data() = 0;
  virtual int get_pps_size() = 0;
  virtual AVCodecContext *getCodecContext() = 0;
};

#endif // _IVIDEO_ENCODER_H_
