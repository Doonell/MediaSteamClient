#ifndef _IRTMP_PROTOCOL_H_
#define _IRTMP_PROTOCOL_H_

#include <cstdint>
#include <string>

class IRTMPProtocol {
public:
  virtual ~IRTMPProtocol() = default;
  virtual int setupConnection(const std::string &url) = 0;
  virtual int sendMetaData() = 0;
  virtual int sendAudioSpecificConfig(const uint8_t *data, int size) = 0;
  virtual int sendAudioRawData(const uint8_t *data, int size,
                               int timestamp) = 0;
  virtual int sendH264SequenceHeader(const uint8_t *data, int size,
                                     int timestamp) = 0;
  virtual int sendH264RawData(const uint8_t *data, int size, int timestamp) = 0;
  virtual int sendPacket(RTMPPacket *packet, int queue) = 0;
};

#endif // _IRTMP_PROTOCOL_H_
