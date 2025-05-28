#ifndef _RTMP_HOLDER_H_
#define _RTMP_HOLDER_H_

#include "librtmp/rtmp.h"
#include <memory>

class RTMPHolder {
public:
  RTMPHolder() {
    rtmp_ = RTMP_Alloc();
    RTMP_Init(rtmp_);
  }
  ~RTMPHolder() { RTMP_Free(rtmp_); }

  RTMP *get() const { return rtmp_; }

  void setConnectionTimeout(int timeout) { rtmp_->Link.timeout = timeout; }

  bool setUrl(const std::string &url) {
    if (RTMP_SetupURL(rtmp_, url.c_str()) < 0) {
      return false;
    }
    return true;
  }

  void setLive(bool live) {
    if (live) {
      rtmp_->Link.lFlags |= RTMP_LF_LIVE; // 设置为直播流
    } else {
      rtmp_->Link.lFlags &= ~RTMP_LF_LIVE; // 取消直播流设置
    }
  }

  void setBufferMS(int ms) { RTMP_SetBufferMS(rtmp_, ms); }

private:
  RTMP *rtmp_;
};

#endif // _RTMP_HOLDER_H_
