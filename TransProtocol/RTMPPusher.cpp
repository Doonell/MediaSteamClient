#include "RTMPPusher.h"
#include "AVTagDataBuilder.h"
#include "Queue/MsgQueue.h"
#include "RTMPMessageFormatBuilder.h"
#include "RTMPPacketBuilder.h"
#include "Util/TimeHelper.h"

namespace TransProtocol {

void RTMPPusher::start() {
  auto msg = msgQueue_.recvMessage();
  std::visit([&](auto &&msg) { handleMessage(msg); }, *msg);
}

void RTMPPusher::handleMessage(VideoSequenceMessage &msg) {
  sendH264SequenceHeader(msg);
}

void RTMPPusher::handleMessage(H264RawMessage &msg) {
  AVTagDataBuilder h264TagData(1024);

  if (msg.getFrameType() == H264RawMessage::frameType::Keyframe) {
    h264TagData.setFrameType(AVTagDataBuilder::FrameType::Keyframe); // I 帧
  } else {
    h264TagData.setFrameType(
        AVTagDataBuilder::FrameType::Interframe); // P帧，B帧
  }

  h264TagData.setCodecId(AVTagDataBuilder::CodecId::AVC)
      .setCompositionTime(0)
      .setNaluLength(msg.nalu_size_)
      .setNaluData(msg.nalu_, msg.nalu_size_)
      .buildRawData();

  time_ = TimeHelper::GetTimeMillisecond();
  return sendPacket(RTMP_PACKET_TYPE_VIDEO, h264TagData.data(),
                    h264TagData.size(), time_);
}

void RTMPPusher::handleMessage(AudioSpecificConfig &msg) {
  AVTagDataBuilder audioSpecificConfig();

  audioSpecificConfig
      .setSoundFormat(AVTagDataBuilder::ESoundFormat::AAC)        // AAC
      .setSoundRate(AVTagDataBuilder::ESoundRate::RATE_44_KHZ)    // 44.1KHz
      .setSoundSize(AVTagDataBuilder::ESoundSize::SoundSize16Bit) // 16bit
      .setSoundType(AVTagDataBuilder::ESoundType::SoundStereo)    // Stereo
      .setPacketType(AVTagDataBuilder::AVCPacketType::
                         SEQUENCE_HEADER) // AudioSpecificConfig
      .setAudioObjectType(AVTagDataBuilder::EAudioObjectType::AAC_LC) // AAC LC
      .setSamplingFreqIndex(
          AVTagDataBuilder::ESamplingFrequencyIndex::FREQ_44100_HZ) // 44.1KHz
      .setChannelConfiguration(
          AVTagDataBuilder::EChannelConfiguration::STEREO) // Stereo
      .setFrameLengthFlag(
          AVTagDataBuilder::EFrameLengthFlag::FRAME_1024_SAMPLES)
      .setDependsOnCoreCoder(
          AVTagDataBuilder::EDependsOnCoreCoder::NO_DEPENDENCY)
      .setExtensionFlag(AVTagDataBuilder::ExtensionFlag::NO_EXTENSION)
      .buildAudioSpecificConfig();

  timestamp_ = TimeHelper::GetTimeMillisecond();

  RTMPPacketBuilder rtmpPacket;
  rtmpPacket.chunkAudioChannel()
      .chunkFormatLarge()
      .chunkTimeStamp(timestamp_)
      .chunkStreamId(rtmp_->m_stream_id)
      .MessageType(RTMP_PACKET_TYPE_AUDIO)
      .chunkBodySize(audioSpecificConfig.size())
      .RtmpData(audioSpecificConfig.data(), audioSpecificConfig.size());

  int nRet = RTMP_SendPacket(rtmp_, rtmpPacket.data(), 0);
  if (nRet != 1) {
    LOG_INFO("RTMP_SendPacket fail\n");
  }
}

void RTMPPusher::handleMessage(AudioRawDataMessage &msg) {
  AVTagDataBuilder audioRawData(1024);
  audioRawData
      .setSoundFormat(AVTagDataBuilder::ESoundFormat::AAC)        // AAC
      .setSoundRate(AVTagDataBuilder::ESoundRate::RATE_44_KHZ)    // 44.1KHz
      .setSoundSize(AVTagDataBuilder::ESoundSize::SoundSize16Bit) // 16bit
      .setSoundType(AVTagDataBuilder::ESoundType::SoundStereo)    // Stereo
      .setPacketType(AVTagDataBuilder::AVCPacketType::
                         SEQUENCE_HEADER) // AudioSpecificConfig
      .setAudioData(msg.data_, msg.size_)
      .buildAudioRawData();
  timestamp_ = TimeHelper::GetTimeMillisecond();

  RTMPPacketBuilder rtmpPacket;
  rtmpPacket.chunkAudioChannel()
      .chunkFormatLarge()
      .chunkTimeStamp(timestamp_)
      .chunkStreamId(rtmp_->m_stream_id)
      .MessageType(RTMP_PACKET_TYPE_AUDIO)
      .chunkBodySize(audioRawData.size())
      .RtmpData(audioRawData.data(), audioRawData.size());

  int nRet = RTMP_SendPacket(rtmp_, rtmpPacket.data(), 0);
  if (nRet != 1) {
    LOG_INFO("RTMP_SendPacket fail\n");
  }
}

bool RTMPPusher::sendH264SequenceHeader(VideoSequenceHeaderMsg *seq_header) {
  if (seq_header == NULL) {
    return false;
  }

  AVTagDataBuilder h264SeqenceHeader(1024);
  h264SeqenceHeader.setCodecId(AVTagDataBuilder::CodecId::AVC)
      .setFrameType(AVTagDataBuilder::FrameType::Keyframe)
      .setCompositionTime(0)
      .setConfigurationVersion(1)
      .setAVCProfileIndication(seq_header->sps_[1])
      .setProfileCompatibility(seq_header->sps_[2])
      .setAVCLevelIndication(seq_header->sps_[3])
      .setLengthSizeMinusOne(0xff)
      .setNumOfSps(1)
      .setSpsLength(seq_header->sps_size_)
      .setSpsData(seq_header->sps_, seq_header->sps_size_)
      .setNumOfPps(1)
      .setPpsLength(seq_header->pps_size_)
      .setPpsData(seq_header->pps_, seq_header->pps_size_)
      .build();
  time_ = TimeHelper::GetTimeMillisecond();
  return sendPacket(RTMP_PACKET_TYPE_VIDEO, h264SeqenceHeader.data(),
                    h264SeqenceHeader.size(), 0);
}

bool RTMPPusher::SendMetadata(FLVMetaMessage *metadata) {
  if (metadata == NULL) {
    return false;
  }

  RTMPMessageFormatBuilder metaBuilder;
  metaBuilder.put_byte(AMF_STRING)
      .put_amf_string("@setDataFrame")
      .put_amf_string("onMetaData")
      .put_byte(AMF_OBJECT)
      .put_amf_string("copyright")
      .put_byte(AMF_STRING)
      .put_amf_string("firehood")
      .put_amf_string("width")
      .put_amf_double(metadata->width)
      .put_amf_string("height")
      .put_amf_double(metadata->height)
      .put_amf_string("framerate")
      .put_amf_double(metadata->framerate)
      .put_amf_string("videodatarate")
      .put_amf_double(metadata->videodatarate)
      .put_amf_string("videocodecid")
      .put_amf_double(FLV_CODECID_H264)
      .put_amf_string("audiodatarate")
      .put_amf_double(metadata->audiodatarate)
      .put_amf_string("audiosamplerate")
      .put_amf_double(metadata->audiosamplerate)
      .put_amf_string("audiosamplesize")
      .put_amf_double(metadata->audiosamplesize)
      .put_amf_string("stereo")
      .put_amf_double(metadata->channles)
      .put_amf_string("audiocodecid")
      .put_amf_double(FLV_CODECID_AAC)
      .put_amf_string("")
      .put_byte(AMF_OBJECT_END);

  return sendPacket(RTMP_PACKET_TYPE_INFO, metaBuilder.data(),
                    metaBuilder.size(), 0);
}

int RTMPPusher::sendPacket(unsigned int packet_type, unsigned char *data,
                           unsigned int size, unsigned int timestamp) {
  if (rtmp_ == nullptr) {
    return -1;
  }

  auto packetBuilder = std::make_shared<RTMPPacketBuilder>(size);
  packetBuilder->chunkAudioChannel()
      .chunkFormatLarge()
      .chunkTimeStamp(timestamp)
      .chunkStreamId(rtmp_->m_stream_id)
      .MessageType(packet_type)
      .chunkBodySize(size)
      .RtmpData(data, size);

  int nRet = RTMP_SendPacket(rtmp_, packetBuilder->get(), 0);
  if (nRet != 1) {
    LOG_INFO("RTMP_SendPacket fail\n");
  }

  return nRet;
}

void RTMPPusher::stop() {}
void RTMPPusher::send(const std::string &data) {}
void RTMPPusher::recv(const std::string &data) {}

} // namespace TransProtocol