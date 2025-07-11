#ifndef _IRTMP_PROTOCOL_H_
#define _IRTMP_PROTOCOL_H_

#include <cstdint>
#include <string>

namespace TransProtocol {
enum class EFrameType { KeyFrame = 0, InterFrame = 1, Metadata = 2 };
class IRTMPProtocol {
public:
  virtual ~IRTMPProtocol() = default;
  virtual bool init() = 0;
  virtual bool connect() = 0;
  virtual bool isConnected() = 0;
  virtual int sendMetaData(double width, double height, double framerate,
                           double videodatarate, double audiodatarate,
                           double audiosamplerate, double audiosamplesize,
                           double channels) = 0;
  virtual void sendAudioSpecificConfig(uint8_t profile, uint8_t channels,
                                       uint32_t sample_rate) = 0;
  virtual int sendAudioRawData(uint8_t *data, int size) = 0;
  virtual int sendH264SequenceHeader(uint8_t *sps, uint32_t sps_size,
                                     uint8_t *pps, uint32_t pps_size) = 0;
  virtual int sendH264RawData(bool, uint8_t *data, int size) = 0;
  virtual int sendPacket(unsigned int packet_type, unsigned char *data,
                         unsigned int size, int64_t timestamp) = 0;
};
} // namespace TransProtocol
#endif // _IRTMP_PROTOCOL_H_
