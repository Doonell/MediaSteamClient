#ifndef _YUV_FILEREADER_H_
#define _YUV_FILEREADER_H_

#include "../Logger/Logger.h"
#include "../Util/TimeHelper.h"
#include "../Util/timeUtil.h"
#include <functional>
#include <string>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#ifdef __cplusplus
};
#endif

class YUVFileReader {
public:
  YUVFileReader(const std::string &yuvFilepath,
                int pixelFormat = AV_PIX_FMT_YUV420P, uint32_t fps = 25,
                uint32_t width = 720, uint32_t height = 480);
  ~YUVFileReader();

  bool init();

  template <typename Callback> void start(Callback handleYUV) {
    LOG_INFO("into loop");
    /*
    Y: width × height
    U: width × height / 4
    V: width × height / 4
    合计：width × height × (1 + 0.25 + 0.25) = width × height × 1.5
    */
    yuv_buf_size = width_ * height_ * 1.5;
    yuv_buf_ = new uint8_t[yuv_buf_size];

    yuv_total_duration_ = 0;
    yuv_start_time_ = Time::TimesUtil::GetTimeMillisecond();
    LOG_INFO("into loop while");

    while (true) {
      if (request_exit_)
        break;
      if (readYuvFile(yuv_buf_, yuv_buf_size) == 0) {
        if (!is_first_frame_) {
          is_first_frame_ = true;
          LOG_INFO("%s:t%u", AVPublishTime::GetInstance()->getVInTag(),
                   AVPublishTime::GetInstance()->getCurrenTime());
        }
        handleYUV(yuv_buf_, yuv_buf_size);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    closeYuvFile();
  }

private:
  int openYuvFile(const char *file_name);
  int readYuvFile(uint8_t *yuv_buf, int32_t yuv_buf_size);
  int closeYuvFile();

private:
  std::string input_yuv_name_;
  int pixel_format_ = 0;
  int fps_;
  int width_ = 0;
  int height_ = 0;

  double frame_duration_ = 40;
  int64_t yuv_start_time_ = 0;    // 起始时间
  double yuv_total_duration_ = 0; // PCM读取累计的时间
  FILE *yuv_fp_ = nullptr;
  uint8_t *yuv_buf_ = nullptr;
  int yuv_buf_size = 0;
  bool is_first_frame_ = false;
  bool request_exit_ = false;
  std::function<void(uint8_t *, int)> handleData_;
};

#endif // VIDEOCAPTURER_H
