#ifndef _TIME_HELPER_H_
#define _TIME_HELPER_H_

#include <chrono>

class TimeHelper {
public:
  static uint64_t GetTimeMillisecond() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }
};

#endif //_TIME_HELPER_H_
