#include "PCMFileReader.h"
#include "../Logger/Logger.h"
#include "../Util/TimeHelper.h"
#include "../Util/timeUtil.h"

namespace Reader {

PCMFileReader::PCMFileReader(const std::string &filePath, uint32_t sampleRate)
    : filePath_(filePath), sampleRate_(sampleRate) {}

PCMFileReader::~PCMFileReader() {
  if (pcm_fp_)
    fclose(pcm_fp_);
}

bool PCMFileReader::init() {
  if (openPcmFile() != 0) {
    std::cout << "openPcmFile %s failed " << filePath_.c_str() << std::endl;
    return false;
  }
  return true;
}

int PCMFileReader::openPcmFile() {
  pcm_fp_ = fopen(filePath_.c_str(), "rb");
  if (!pcm_fp_) {
    return -1;
  }
  return 0;
}

int PCMFileReader::readPcmFile(uint8_t *pcm_buf, int32_t nb_samples) {
  int64_t cur_time = Time::TimesUtil::GetTimeMillisecond();
  int64_t dif = cur_time - pcm_start_time_;

  if ((int64_t)pcm_total_duration_ > dif)
    return -1;

  if (!pcm_fp_) {
      std::cout << "pcm file is null \n";
      return -1;
  }

  size_t ret = ::fread(pcm_buf, 1, nb_samples * 4, pcm_fp_);
  if (ret != nb_samples * 4) {
    // 从文件头部开始读取
    ret = ::fseek(pcm_fp_, 0, SEEK_SET);
    ret = ::fread(pcm_buf, 1, nb_samples * 4, pcm_fp_);
    if (ret != nb_samples * 4) {
      return -1;
    }
  }

  pcm_total_duration_ += (nb_samples * 1.0 / 48);
  return 0;
}

int PCMFileReader::closePcmFile() {
  if (pcm_fp_)
    fclose(pcm_fp_);
  return 0;
}
} // namespace Reader
