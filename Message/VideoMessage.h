#ifndef _VIDEO_MESSAGE_H_
#define _VIDEO_MESSAGE_H_

#include "../Logger/Logger.h"
#include <cstddef> // size_t
#include <cstdint>
#include <map>
#include <sstream>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
// #include <string>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif
#include <ctype.h>

#include "librtmp/rtmp.h"

namespace Message {
struct VideoSequenceMessage {
public:
  VideoSequenceMessage(uint8_t *sps, int sps_size, uint8_t *pps, int pps_size) {
    sps_ = (uint8_t *)malloc(sps_size * sizeof(uint8_t));
    pps_ = (uint8_t *)malloc(pps_size * sizeof(uint8_t));
    if (!sps_ || !pps_) {
      LOG_INFO("VideoSequenceMessage malloc failed");
      return;
    }
    sps_size_ = sps_size;
    memcpy(sps_, sps, sps_size);
    pps_size_ = pps_size;
    memcpy(pps_, pps, pps_size);
  }

  ~VideoSequenceMessage() {
    if (sps_)
      free(sps_);
    if (pps_)
      free(pps_);
  }
  uint8_t *sps_;
  int sps_size_;
  uint8_t *pps_;
  int pps_size_;
  unsigned int nWidth;
  unsigned int nHeight;
  unsigned int nFrameRate;     // fps
  unsigned int nVideoDataRate; // bps
  int64_t pts_ = 0;
};
} // namespace Message
#endif //_VIDEO_MESSAGE_H_
