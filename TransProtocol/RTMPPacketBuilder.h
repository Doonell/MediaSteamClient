#ifndef _RTMP_PACKET_BUILDER_H_
#define _RTMP_PACKET_BUILDER_H_

#include "librtmp/rtmp.h"

class RTMPPacketBuilder {
public:
  RTMPPacketBuilder(uint32_t size) {
    RTMPPacket_Reset(&rtmpPacket_);
    RTMPPacket_Alloc(&rtmpPacket_, size);
  }

  ~RTMPPacketBuilder() { RTMPPacket_Free(&rtmpPacket_); }

  RTMPPacketBuilder &chunkAudioChannel() {
    rtmpPacket_.m_nChannel = RTMP_AUDIO_CHANNEL;
    return *this;
  }

  RTMPPacketBuilder &chunkVideoChannel() {
    rtmpPacket_.m_nChannel = RTMP_VIDEO_CHANNEL;
    return *this;
  }

  RTMPPacketBuilder &MessageType(uint32_t messageType) {
    rtmpPacket_.m_packetType = messageType;
    return *this;
  }

  RTMPPacketBuilder &chunkMetadataChannel() {
    rtmpPacket_.m_nChannel = RTMP_METADATA_CHANNEL;
    return *this;
  }

  RTMPPacketBuilder &chunkFormatLarge() {
    rtmpPacket_.m_headerType = RTMP_PACKET_SIZE_LARGE;
    return *this;
  }

  RTMPPacketBuilder &chunkTimeStamp(unsigned int timestamp) {
    rtmpPacket_.m_nTimeStamp = timestamp;
    return *this;
  }

  RTMPPacketBuilder &chunkStreamId(unsigned int cstreamId) {
    rtmpPacket_.m_nInfoField2 = cstreamId;
    return *this;
  }

  RTMPPacketBuilder &chunkBodySize(unsigned int bodySize) {
    rtmpPacket_.m_nBodySize = bodySize;
    return *this;
  }

  RTMPPacketBuilder &RtmpData(unsigned char *data, unsigned int size) {
    memcpy(rtmpPacket_.m_body, data, size);
    return *this;
  }

  RTMPPacket *data() { return &rtmpPacket_; }

private:
  RTMPPacket rtmpPacket_;
};

#endif //_RTMP_PACKET_BUILDER_H_
