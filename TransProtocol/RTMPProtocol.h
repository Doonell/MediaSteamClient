#ifndef _RTMP_PROTOCOL_H_
#define _RTMP_PROTOCOL_H_

#include "IRTMPProtocol.h"
#include "RTMPHolder.h"

namespace TransProtocol {

enum ERTMP_BASE_TYPE {
  RTMP_BASE_TYPE_UNKNOW,
  RTMP_BASE_TYPE_PLAY,
  RTMP_BASE_TYPE_PUSH
};

enum EFLV_CODECID {
  FLV_CODECID_H264 = 7,
  FLV_CODECID_AAC = 10,
};
class RTMPProtocol : public IRTMPProtocol {
public:
  RTMPProtocol(const std::string &url,
               ERTMP_BASE_TYPE rtmp_obj_type = RTMP_BASE_TYPE_PUSH,
               bool enableVideo = true, bool enableAudio = true)
      : url_(url), rtmp_obj_type_(rtmp_obj_type), enable_video_(enableVideo),
        enable_audio_(enableAudio) {}
  ~RTMPProtocol();

  bool init() override;
  bool connect() override;
  bool isConnected() override;
  bool isReadCompleted(RTMPPacket *packet) override;

  int sendMetaData(double width, double height, double framerate,
                   double videodatarate, double audiodatarate,
                   double audiosamplerate, double audiosamplesize,
                   double channels) override;
  void sendAudioSpecificConfig(uint8_t profile, uint8_t channels,
                               uint32_t sample_rate) override;
  int sendAudioRawData(uint8_t *data, int size, uint32_t pts) override;
  int sendH264SequenceHeader(uint8_t *sps, uint32_t sps_size, uint8_t *pps,
                             uint32_t pps_size) override;
  int sendH264RawData(bool, uint8_t *, int, uint32_t) override;
  int sendPacket(unsigned int packet_type, unsigned char *data,
                 unsigned int size, int64_t timestamp) override;
  bool readPacket(RTMPPacket *packet) override;
  void respondPacket(RTMPPacket *packet) override;
  void freePacket(RTMPPacket *packet) override;

private:
  std::string url_;
  ERTMP_BASE_TYPE rtmp_obj_type_;
  bool enable_video_;
  bool enable_audio_;
  RTMPHolder rtmpHolder_;
  int64_t timestamp_;
};
} // namespace TransProtocol
#endif // _RTMP_PROTOCOL_H_