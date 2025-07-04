#ifndef _PCM_FILE_READER_H_
#define _PCM_FILE_READER_H_

#include "../Logger/Logger.h"
#include "IReader.h"
#include <array>

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

const int PCM_BUF_MAX_SIZE = 32768;
namespace Reader {
class PCMFileReader {
public:
  PCMFileReader(const std::string &filePath = "buweishui_48000_2_s16le.pcm",
                uint32_t sampleRate = 48000);
  ~PCMFileReader();
  bool init();

  template <typename Callback> void start(Callback handlePcm) {
    LOG_INFO("into loop");

    int nb_samples = 1024;
    std::array<uint8_t, PCM_BUF_MAX_SIZE> pcm_buf_;
    pcm_total_duration_ = 0;
    pcm_start_time_ = Time::TimesUtil::GetTimeMillisecond();
    LOG_INFO("into loop while");
    while (true) {
      if (readPcmFile(pcm_buf_.data(), nb_samples) == 0) {
        if (!is_first_frame_) {
          is_first_frame_ = true;
          LOG_INFO("%s:t%u",
                   TimeHelper::AVPublishTime::GetInstance()->getAInTag(),
                   TimeHelper::AVPublishTime::GetInstance()->getCurrenTime());
        }
        handlePcm(pcm_buf_.data(), nb_samples * 4); // 2通道 s16格式
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    closePcmFile();
  }

private:
  int openPcmFile();
  int readPcmFile(uint8_t *pcm_buf, int32_t pcm_buf_size);
  int closePcmFile();

private:
  std::string filePath_;
  uint32_t sampleRate_;
  int64_t pcmStartTime_ = 0;    // 起始时间
  double pcmTotalDuration_ = 0; // PCM读取累计的时间
  FILE *pcmFp_ = nullptr;
  bool isFirstFrame_ = false;
  int64_t pcm_start_time_ = 0;    // 起始时间
  double pcm_total_duration_ = 0; // PCM读取累计的时间
  FILE *pcm_fp_ = NULL;
  bool is_first_frame_ = false;
};
} // namespace Reader
#endif // _PCM_FILE_READER_H_