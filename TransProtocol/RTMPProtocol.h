#ifndef _RTMP_PROTOCOL_H_
#define _RTMP_PROTOCOL_H_

#include "IRTMPProtocol.h"
#include "RTMPHolder.h"

class RTMPProtocol : public IRTMPProtocol {
public:
  RTMPProtocol(const std::string &url, int streamId, bool enableVideo = true,
               bool enableAudio = true);
  ~RTMPProtocol() = default;

  int setupConnection(const std::string &url) override;
  int sendMetaData() override;
  int sendAudioSpecificConfig(const uint8_t *data, int size) override;
  int sendAudioRawData(const uint8_t *data, int size, int timestamp) override;
  int sendH264SequenceHeader(const uint8_t *data, int size,
                             int timestamp) override;
  int sendH264RawData(const uint8_t *data, int size, int timestamp) override;

private:
  RTMPHolder rtmpHolder_;
};

#endif // _RTMP_PROTOCOL_H_