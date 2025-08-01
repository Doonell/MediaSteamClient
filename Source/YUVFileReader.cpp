#include "YUVFileReader.h"

YUVFileReader::YUVFileReader(const std::string &yuvFilepath, int pixelFormat,
                             uint32_t fps, uint32_t width, uint32_t height)
    : input_yuv_name_(yuvFilepath), fps_(fps), width_(width), height_(height) {}

YUVFileReader::~YUVFileReader() {
  if (yuv_buf_)
    delete[] yuv_buf_;
}

bool YUVFileReader::init() {
  frame_duration_ = 1000.0 / fps_;
  if (openYuvFile(input_yuv_name_.c_str()) != 0) {
    LOG_INFO("openYuvFile %s failed", input_yuv_name_.c_str());
    return false;
  }

  return true;
}

int YUVFileReader::openYuvFile(const char *file_name) {
  yuv_fp_ = fopen(file_name, "rb");
  if (!yuv_fp_) {
    int err = errno;
    printf("fopen failed, errno=%d, error=%s\n", err, strerror(err));
    return -1;
  }
  return 0;
}

int YUVFileReader::readYuvFile(uint8_t *yuv_buf, int32_t yuv_buf_size) {
  int64_t cur_time = Time::TimesUtil::getTimeMillisecond();
  int64_t dif = cur_time - yuv_start_time_;
  if ((int64_t)yuv_total_duration_ > dif)
    return -1;
  // 该读取数据了
  if (!yuv_fp_) {
    std::cout << " file fp is null \n";
    return -1;
  }

  size_t ret = ::fread(yuv_buf, 1, yuv_buf_size, yuv_fp_);
  if (ret != yuv_buf_size) {
    // 从文件头部开始读取
    ret = ::fseek(yuv_fp_, 0, SEEK_SET);
    ret = ::fread(yuv_buf, 1, yuv_buf_size, yuv_fp_);
    if (ret != yuv_buf_size) {
      return -1;
    }
  }
  LOG_DEBUG("yuv_total_duration_" << (int64_t)yuv_total_duration_ << "ms, "
                                  << dif << "ms");
  yuv_total_duration_ += frame_duration_;
  return 0;
}

int YUVFileReader::closeYuvFile() {
  if (yuv_fp_)
    fclose(yuv_fp_);
  return 0;
}
