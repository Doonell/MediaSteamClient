#include "RTMPProtocol.h"
#include "../Logger/Logger.h"
#include "../Util/timeUtil.h"
#include "AVTagDataBuilder.h"
#include "IRTMPProtocol.h"
#include "RTMPMessageFormatBuilder.h"
#include "RTMPPacketBuilder.h"
namespace TransProtocol {

RTMPProtocol::~RTMPProtocol() {
  if (rtmpHolder_.get()) {
    RTMP_Close(rtmpHolder_.get());
  }

  if (isConnected()) {
    RTMP_Close(rtmpHolder_.get());
  }

#ifdef WIN32
  WSACleanup();
#endif // WIN32
}

bool RTMPProtocol::init() {
  bool ret_code = true;
#ifdef WIN32
  WORD version;
  WSADATA wsaData;
  version = MAKEWORD(1, 1);
  ret_code = (WSAStartup(version, &wsaData) == 0) ? true : false;
#endif
  if (!ret_code) {
    LOG_ERROR("WSAStartup failed");
    return false;
  }

  return ret_code;
}

bool RTMPProtocol::connect() {
  LOG_INFO("base begin connect");
  std::cout << "base begin connect \n";
  init();
  rtmpHolder_.setConnectionTimeout(10);
  if (!rtmpHolder_.setUrl(url_)) {
    LOG_INFO("RTMP_SetupURL failed");
    return false;
  }

  rtmpHolder_.setLive(true);
  rtmpHolder_.setBufferMS(3600 * 1000);

  if (rtmp_obj_type_ == RTMP_BASE_TYPE_PUSH)
    RTMP_EnableWrite(rtmpHolder_.get());
  if (!RTMP_Connect(rtmpHolder_.get(), NULL)) {
    std::cout << "RTMP_Connect failed! \n";
    return false;
  }
  if (!RTMP_ConnectStream(rtmpHolder_.get(), 0)) {
    std::cout << "RTMP_ConnectStream failed\n";
    return false;
  }
  // 判断是否打开音视频,默认打开
  if (rtmp_obj_type_ == RTMP_BASE_TYPE_PUSH) {
    if (!enable_video_) {
      RTMP_SendReceiveVideo(rtmpHolder_.get(), enable_video_);
    }
    if (!enable_audio_) {
      RTMP_SendReceiveAudio(rtmpHolder_.get(), enable_audio_);
    }
  }

  return true;
}

bool RTMPProtocol::isConnected() { return RTMP_IsConnected(rtmpHolder_.get()); }

int RTMPProtocol::sendMetaData(double width, double height, double framerate,
                               double videodatarate, double audiodatarate,
                               double audiosamplerate, double audiosamplesize,
                               double channles) {
  RTMPMessageFormatBuilder metaBuilder;
  metaBuilder.put_byte(AMF_STRING)
      .put_amf_string("@setDataFrame")
      .put_byte(AMF_STRING)
      .put_amf_string("onMetaData")
      .put_byte(AMF_OBJECT)
      .put_amf_string("copyright")
      .put_byte(AMF_STRING)
      .put_amf_string("firehood")
      // .put_amf_string("width")
      // .put_amf_double(width)
      // .put_amf_string("height")
      // .put_amf_double(height)
      // .put_amf_string("framerate")
      // .put_amf_double(framerate)
      // .put_amf_string("videodatarate")
      // .put_amf_double(videodatarate)
      // .put_amf_string("videocodecid")
      // .put_amf_double(FLV_CODECID_H264)
      .put_amf_string("audiodatarate")
      .put_amf_double(audiodatarate)
      .put_amf_string("audiosamplerate")
      .put_amf_double(audiosamplerate)
      .put_amf_string("audiosamplesize")
      .put_amf_double(audiosamplesize)
      .put_amf_string("stereo")
      .put_amf_double(channles)
      .put_amf_string("audiocodecid")
      .put_amf_double(FLV_CODECID_AAC)
      .put_amf_string("")
      .put_byte(AMF_OBJECT_END);
  metaBuilder.showAll();
  return sendPacket(RTMP_PACKET_TYPE_INFO, metaBuilder.data(),
                    metaBuilder.size(), 0);
}

void RTMPProtocol::sendAudioSpecificConfig(uint8_t profile, uint8_t channels,
                                           uint32_t sample_rate) {
  AVTagDataBuilder audioSpecificConfig;

  audioSpecificConfig
      .setSoundFormat(AVTagDataBuilder::ESoundFormat::AAC)        // AAC
      .setSoundRate(AVTagDataBuilder::ESoundRate::RATE_44_KHZ)    // 44.1KHz
      .setSoundSize(AVTagDataBuilder::ESoundSize::SoundSize16Bit) // 16bit
      .setSoundType(AVTagDataBuilder::ESoundType::SoundStereo)    // Stereo
      .setAVPacketType(AVTagDataBuilder::AVPacketType::
                           SEQUENCE_HEADER) // AudioSpecificConfig
      .setAudioObjectType(
          static_cast<AVTagDataBuilder::EAudioObjectType>(profile)) // AAC LC
      .setSamplingFreqIndex(sample_rate)                            // 44.1KHz
      .setChannelConfiguration(
          static_cast<AVTagDataBuilder::EChannelConfiguration>(
              channels)) // Stereo
      .setFrameLengthFlag(
          AVTagDataBuilder::EFrameLengthFlag::FRAME_1024_SAMPLES)
      .setDependsOnCoreCoder(
          AVTagDataBuilder::EDependsOnCoreCoder::NO_DEPENDENCY)
      .setExtensionFlag(AVTagDataBuilder::ExtensionFlag::NO_EXTENSION)
      .buildAudioSpecificConfig();

  RTMPPacketBuilder rtmpPacket;
  rtmpPacket.chunkAudioChannel()
      .chunkFormatLarge()
      .chunkTimeStamp(0)
      .chunkStreamId(rtmpHolder_.get()->m_stream_id)
      .MessageType(RTMP_PACKET_TYPE_AUDIO)
      .chunkBodySize(audioSpecificConfig.size())
      .RtmpData(audioSpecificConfig.data(), audioSpecificConfig.size());

  int nRet = RTMP_SendPacket(rtmpHolder_.get(), rtmpPacket.data(), 0);
  if (nRet != 1) {
    LOG_INFO("RTMP_SendPacket fail\n");
  }
}

int RTMPProtocol::sendAudioRawData(uint8_t *data, int size) {
  // AVTagDataBuilder audioRawData;
  // audioRawData
  //     .setSoundFormat(AVTagDataBuilder::ESoundFormat::AAC)        // AAC
  //     .setSoundRate(AVTagDataBuilder::ESoundRate::RATE_44_KHZ)    // 44.1KHz
  //     .setSoundSize(AVTagDataBuilder::ESoundSize::SoundSize16Bit) // 16bit
  //     .setSoundType(AVTagDataBuilder::ESoundType::SoundStereo)    // Stereo
  //     .setAVPacketType(AVTagDataBuilder::AVPacketType::
  //                          SEQUENCE_HEADER) // AudioSpecificConfig
  //     .setAudioData(data, size)
  //     .buildAudioRawData();
  // timestamp_ = Time::TimesUtil::GetTimeMillisecond();

  RTMPPacketBuilder rtmpPacket;
  rtmpPacket.chunkAudioChannel()
      .chunkFormatLarge()
      .chunkTimeStamp(timestamp_)
      .chunkStreamId(rtmpHolder_.get()->m_stream_id)
      .MessageType(RTMP_PACKET_TYPE_AUDIO)
      .chunkBodySize(size)
      .RtmpData(data, size);

  int nRet = RTMP_SendPacket(rtmpHolder_.get(), rtmpPacket.data(), 0);
  if (nRet != 1) {
    LOG_INFO("RTMP_SendPacket fail\n");
  }
  return nRet;
}

int RTMPProtocol::sendH264SequenceHeader(uint8_t *sps, uint32_t sps_size,
                                         uint8_t *pps, uint32_t pps_size) {
  AVTagDataBuilder h264SeqenceHeader;
  h264SeqenceHeader.setCodecId(AVTagDataBuilder::CodecId::AVC)
      .setFrameType(AVTagDataBuilder::FrameType::Keyframe)
      .setAVPacketType(AVTagDataBuilder::AVPacketType::SEQUENCE_HEADER)
      .setCompositionTime(0)
      .setConfigurationVersion(1)
      .setAVCProfileIndication(sps[1])
      .setProfileCompatibility(sps[2])
      .setAVCLevelIndication(sps[3])
      .setLengthSizeMinusOne(0xff)
      .setNumOfSps(1)
      .setSpsLength(sps_size)
      .setSpsData(sps, sps_size)
      .setNumOfPps(1)
      .setPpsLength(pps_size)
      .setPpsData(pps, pps_size)
      .build();
  timestamp_ = Time::TimesUtil::GetTimeMillisecond();
  return sendPacket(RTMP_PACKET_TYPE_VIDEO, h264SeqenceHeader.data(),
                    h264SeqenceHeader.size(), 0);
}

int RTMPProtocol::sendH264RawData(bool isKeyFrameType, uint8_t *nalu,
                                  int nalu_size, uint32_t pts) {
  AVTagDataBuilder h264TagData;

  if (isKeyFrameType) {
    h264TagData.setFrameType(AVTagDataBuilder::FrameType::Keyframe); // I 帧
  } else {
    h264TagData.setFrameType(
        AVTagDataBuilder::FrameType::Interframe); // P帧，B帧
  }

  h264TagData.setCodecId(AVTagDataBuilder::CodecId::AVC)
      .setCompositionTime(0)
      .setNaluLength(nalu_size)
      .setNaluData(nalu, nalu_size)
      .buildVideoRawData();

  std::cout << " sendH264RawData timestamp: " << pts << std::endl;
  return sendPacket(RTMP_PACKET_TYPE_VIDEO, h264TagData.data(),
                    h264TagData.size(), pts);
}

int RTMPProtocol::sendPacket(unsigned int packet_type, unsigned char *data,
                             unsigned int size, int64_t timestamp) {
  if (rtmpHolder_.get() == nullptr) {
    return -1;
  }

  auto packetBuilder = RTMPPacketBuilder(size);
  packetBuilder.chunkAudioChannel()
      .chunkFormatLarge()
      .chunkTimeStamp(timestamp)
      .chunkStreamId(rtmpHolder_.get()->m_stream_id)
      .MessageType(packet_type)
      .chunkBodySize(size)
      .RtmpData(data, size);

  int nRet = RTMP_SendPacket(rtmpHolder_.get(), packetBuilder.data(), 0);
  if (nRet != 1) {
    LOG_INFO("RTMP_SendPacket fail\n");
  }

  return nRet;
}

} // namespace TransProtocol
