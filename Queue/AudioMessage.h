#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <memory>

namespace Queue {

class FLVAudioMessage {
public:
  FLVAudioMessage(const std::shared_ptr<uint8_t[]> &data, int size);
  ~FLVAudioMessage() = default;

private:
  std::shared_ptr<uint8_t[]> data_;
  int size_;
};

class FLVMetaMessage {
public:
  FLVMetaMessage(const std::shared_ptr<uint8_t[]> &data, int size);
  ~FLVMetaMessage() = default;

private:
  std::shared_ptr<uint8_t[]> data_;
  int size_;
};

class H264RawMessage {
public:
  enum class frameType {
    Keyframe,
    NonKeyframe,
  };

  H264RawMessage(const std::shared_ptr<uint8_t[]> &data, int size);
  ~H264RawMessage() = default;

  void setFrameType(frameType type) { frame_type_ = type; }
  frameType getFrameType() const { return frame_type_; }

private:
  std::shared_ptr<uint8_t[]> data_;
  int size_;
};

class AudioSpecificConfigMessage {
public:
  AudioSpecificConfigMessage(const std::shared_ptr<uint8_t[]> &data, int size);
  ~AudioSpecificConfigMessage() = default;

private:
  uint8_t profile_ = 2; // 2 : AAC LC(Low Complexity)
  uint8_t channels_ = 2;
  uint32_t sample_rate_ = 48000;
  int pts_;
};

class AudioRawDataMessage {
public:
  AudioRawDataMessage(const std::shared_ptr<uint8_t[]> &data, int size);
  ~AudioRawDataMessage() = default;
};
} // namespace Queue
#endif // _MESSAGE_H_