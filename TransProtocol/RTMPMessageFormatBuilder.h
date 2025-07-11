#ifndef _RTMP_MESSAGE_FORMAT_BUILDER_H_
#define _RTMP_MESSAGE_FORMAT_BUILDER_H_

#include "librtmp/rtmp.h"
#include <iomanip>  // std::hex, std::setw, std::setfill

class RTMPMessageFormatBuilder {
public:
  RTMPMessageFormatBuilder(uint32_t size = 1024) : bufferSize_(size) {
    output_ = std::make_unique<uint8_t[]>(bufferSize_);
    bufferOffset_ = 0;
  }
  ~RTMPMessageFormatBuilder() = default;

  uint8_t *data() { return output_.get(); }

  uint32_t size() { return bufferOffset_; }

  RTMPMessageFormatBuilder &put_byte(uint8_t nVal) {
    output_[bufferOffset_] = nVal;
    bufferOffset_++;
    showAll();
    return *this;
  }

  RTMPMessageFormatBuilder &put_be16(uint16_t nVal) {
    output_[bufferOffset_ + 1] = nVal & 0xff;
    output_[bufferOffset_] = nVal >> 8;
    bufferOffset_ += 2;
    return *this;
  }

  RTMPMessageFormatBuilder &put_be24(uint32_t nVal) {
    output_[bufferOffset_ + 2] = nVal & 0xff;
    output_[bufferOffset_ + 1] = nVal >> 8;
    output_[bufferOffset_] = nVal >> 16;
    bufferOffset_ += 3;
    return *this;
  }

  RTMPMessageFormatBuilder &put_be32(uint32_t nVal) {
    output_[bufferOffset_ + 3] = nVal & 0xff;
    output_[bufferOffset_ + 2] = nVal >> 8;
    output_[bufferOffset_ + 1] = nVal >> 16;
    output_[bufferOffset_] = nVal >> 24;
    bufferOffset_ += 4;
    return *this;
  }

  RTMPMessageFormatBuilder &put_be64(uint64_t nVal) {
    put_be32(nVal >> 32);
    put_be32(nVal);
    return *this;
  }

  RTMPMessageFormatBuilder &put_amf_string(const char *str) {
    uint16_t len = strlen(str);
    put_be16(len);
    memcpy(output_.get() + bufferOffset_, str, len);
    bufferOffset_ += len;
    showAll();
    return *this;
  }

  RTMPMessageFormatBuilder &put_amf_double(double d) {
    *(output_.get() + bufferOffset_) = AMF_NUMBER; /* type: Number */
    bufferOffset_++;
    uint8_t *ci, *co;
    ci = (uint8_t *)&d;
    co = output_.get() + bufferOffset_;
    co[0] = ci[7];
    co[1] = ci[6];
    co[2] = ci[5];
    co[3] = ci[4];
    co[4] = ci[3];
    co[5] = ci[2];
    co[6] = ci[1];
    co[7] = ci[0];
    bufferOffset_ += 8;
    return *this;
  }

  void showAll() {
      //// 打印前 n 个值
      //size_t n = bufferOffset_;  // 你要打印的数量
      //for (size_t i = 0; i < n; i++) {
      //    uint8_t val = output_[i];
      //    std::cout << "output_[" << i << "] = "
      //        << "char: '" << (std::isprint(val) ? static_cast<char>(val) : '.') << "', "
      //        << "hex: 0x"
      //        << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(val)
      //        << std::dec << std::endl;
      //}
      //std::cout << "----------------------------" << std::endl;
  }

private:
  uint32_t bufferSize_;
  uint32_t bufferOffset_;
  std::unique_ptr<uint8_t[]> output_;
};

#endif //_RTMP_MESSAGE_FORMAT_BUILDER_H_
