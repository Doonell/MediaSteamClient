#ifndef _RTMP_PACKET_BUILDER_H_
#define _RTMP_PACKET_BUILDER_H_

#include "librtmp/rtmp.h"

class RTMPPacketBuilder {
public:
  enum RTMPChannel {
    RTMP_NETWORK_CHANNEL = 2, ///< channel for network-related messages
                              ///< (bandwidth report, ping, etc)
    RTMP_SYSTEM_CHANNEL,      ///< channel for sending server control messages
    RTMP_AUDIO_CHANNEL,       ///< channel for audio data
    RTMP_VIDEO_CHANNEL = 6,   ///< channel for video data
    RTMP_SOURCE_CHANNEL = 8,  ///< channel for a/v invokes
  };

  RTMPPacketBuilder(uint32_t size = 1024) {
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
