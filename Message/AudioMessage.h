#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <memory>
#include <string>

namespace Message {

class FLVAudioMessage {
public:
  FLVAudioMessage(const std::shared_ptr<uint8_t[]> &data, int size);
  ~FLVAudioMessage() = default;

private:
  std::shared_ptr<uint8_t[]> data_;
  int size_;
};

struct FLVMetaMessage {
  bool has_audio = false;
  bool has_video = false;
  int audiocodeid = -1;
  int audiodatarate = 0;
  int audiodelay = 0;
  int audiosamplerate = 0;
  int audiosamplesize = 0;
  int channles;

  bool canSeekToEnd = 0;

  std::string creationdate;
  int duration = 0;
  int64_t filesize = 0;
  double framerate = 0;
  int height = 0;
  bool stereo = true;

  int videocodecid = -1;
  int64_t videodatarate = 0;
  int width = 0;
  int64_t pts = 0;
};

class H264RawMessage {
public:
  H264RawMessage(const std::shared_ptr<uint8_t[]> &data, int size,
                 int frame_type = 0, uint32_t pts = 0)
      : nalu_(data), nalu_size_(size), frame_type_(frame_type), pts_(pts) {}
  ~H264RawMessage() = default;

  bool isKeyFrame() const { return (frame_type_ == 0x05) ? true : false; }
  int getSize() const { return nalu_size_; }
  uint8_t *getData() const { return nalu_.get(); }
  uint32_t pts() const { return pts_; }

private:
  std::shared_ptr<uint8_t[]> nalu_;
  int nalu_size_;
  int frame_type_;
  uint32_t pts_;
};

class AudioSpecificConfigMessage {
public:
  AudioSpecificConfigMessage(uint8_t profile, uint8_t channel_num,
                             uint32_t samplerate, int pts = 0)
      : profile_(profile), channels_(channel_num), sample_rate_(samplerate),
        pts_(pts) {}
  ~AudioSpecificConfigMessage() = default;

  uint8_t profile_ = 2; // 2 : AAC LC(Low Complexity)
  uint8_t channels_ = 2;
  uint32_t sample_rate_ = 48000;
  int pts_;
};

class AudioRawDataMessage {
public:
  AudioRawDataMessage(int size, int with_adts = 0) {
    size_ = size;
    type = 0;
    with_adts_ = with_adts;
    data_ = (unsigned char *)malloc(size * sizeof(char));
  }

  AudioRawDataMessage(const unsigned char *buf, int bufLen, int with_adts = 0) {
    this->size_ = bufLen;
    type = buf[4] & 0x1f;
    with_adts_ = with_adts;
    data_ = (unsigned char *)malloc(bufLen * sizeof(char));
    memcpy(data_, buf, bufLen);
  }

  ~AudioRawDataMessage() {
    if (data_)
      free(data_);
  }
  int type;
  int size_;
  int with_adts_ = 0;
  unsigned char *data_ = NULL;
  uint32_t pts;
};
} // namespace Message
#endif // _MESSAGE_H_