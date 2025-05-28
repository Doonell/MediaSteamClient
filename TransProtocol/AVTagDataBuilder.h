#ifndef _AV_TAG_DATA_BUILDER_H_
#define _AV_TAG_DATA_BUILDER_H_

class AVTagDataBuilder {
public:
  enum class ESoundFormat : uint8_t {
    LinearPCMPlatformEndian = 0, // 本地字节序的 PCM 编码
    ADPCM = 1,                   // Flash 使用的自定义 ADPCM
    MP3 = 2,                     // MPEG Layer 3 音频
    LinearPCMLittleEndian = 3,   // 小端字节序的 PCM
    Nellymoser16kMono = 4,       // 16kHz 单声道 Nellymoser
    Nellymoser8kMono = 5,        // 8kHz 单声道 Nellymoser
    Nellymoser = 6,              // Nellymoser
    G711ALaw = 7,                // ITU-T G.711 A-law 编码
    G711MuLaw = 8,               // ITU-T G.711 mu-law 编码
    Reserved = 9,                // 保留
    AAC = 10,                    // 高级音频编码（AAC）
    Speex = 11,                  // Speex 语音编码
    MP3_8kHz = 12,               // 8kHz MP3
    DeviceSpecific = 13          // 设备专用
  };

  enum class ESoundRate : uint8_t {
    RATE_5_5_KHZ = 0, // 5.5 kHz
    RATE_11_KHZ = 1,  // 11 kHz
    RATE_22_KHZ = 2,  // 22 kHz
    RATE_44_KHZ = 3   // 44.1 kHz
  };

  enum class ESoundSize : uint8_t {
    SoundSize8Bit = 0x00,
    SoundSize16Bit,
  };

  enum class ESoundType : uint8_t {
    SoundMono = 0x00, // 1 channel
    SoundStereo,      // 2 channels
  };

  enum class EAudioObjectType : uint8_t {
    AAC_MAIN = 1,     // Main profile
    AAC_LC = 2,       // Low Complexity profile（最常用）
    AAC_SSR = 3,      // Scalable Sample Rate profile
    AAC_LTP = 4,      // Long Term Prediction profile
    SBR = 5,          // Spectral Band Replication（通常用于 HE-AAC）
    AAC_SCALABLE = 6, // Scalable AAC
    TWINVQ = 7,       // TwinVQ
    CELP = 8,         // Code Excited Linear Prediction
    HVXC = 9,         // Harmonic Vector eXcitation Coding
    RESERVED_10 = 10,
    RESERVED_11 = 11,
    TTSI = 12, // Text-To-Speech Interface
    MAIN_SYNTHETIC = 13,
    WAVETABLE_SYNTHESIS = 14,
    GENERAL_MIDI = 15,
    ALGORITHMIC_SYNTHESIS = 16,
    ER_AAC_LC = 17, // Error Resilient AAC-LC
    RESERVED_18 = 18,
    ER_AAC_LTP = 19,
    ER_AAC_SCALABLE = 20,
    ER_TWINVQ = 21,
    ER_BSAC = 22,
    ER_AAC_LD = 23,
    ER_CELP = 24,
    ER_HVXC = 25,
    ER_HILN = 26,
    ER_PARAMETRIC = 27,
    SSC = 28,
    PS = 29, // Parametric Stereo（HE-AAC v2 使用）
    MPEG_SURROUND = 30,
    ESCAPE = 31 // 表示实际值在后面扩展字段中（少见）
  };

  enum class ESamplingFrequencyIndex : uint8_t {
    FREQ_96000_HZ = 0,
    FREQ_88200_HZ = 1,
    FREQ_64000_HZ = 2,
    FREQ_48000_HZ = 3,
    FREQ_44100_HZ = 4, // 最常见
    FREQ_32000_HZ = 5,
    FREQ_24000_HZ = 6,
    FREQ_22050_HZ = 7,
    FREQ_16000_HZ = 8,
    FREQ_12000_HZ = 9,
    FREQ_11025_HZ = 10,
    FREQ_8000_HZ = 11,
    FREQ_7350_HZ = 12,
    RESERVED_13 = 13,
    RESERVED_14 = 14,
    ESCAPE = 15 // 表示实际频率在后续字段中（极少见）
  };

  enum class EChannelConfiguration : uint8_t {
    DEFINED_IN_AOT_SPECIFIC_CONFIG =
        0,         // 不在这里指定，具体信息在 Audio Object Type 的扩展字段中
    MONO = 1,      // 1 channel: C (center)
    STEREO = 2,    // 2 channels: L + R
    THREE = 3,     // 3 channels: C + L + R
    FOUR = 4,      // 4 channels: C + L + R + back center
    FIVE = 5,      // 5 channels: C + L + R + back left + back right
    FIVE_ONE = 6,  // 6 channels: C + L + R + back left + back right + LFE (5.1)
    SEVEN_ONE = 7, // 8 channels: C + L + R + back left + back right + side left
  };

  enum class EFrameLengthFlag : uint8_t {
    FRAME_1024_SAMPLES = 0, // 每帧 1024 个采样点（标准）
    FRAME_960_SAMPLES = 1   // 每帧 960 个采样点（低延迟）
  };

  enum class EDependsOnCoreCoder : uint8_t {
    NO_DEPENDENCY = 0,  // 不依赖任何核心编码器（常见）
    DEPENDS_ON_CORE = 1 // 依赖于某个 core coder，需要另一个 stream 解码
  };

  enum class ExtensionFlag : uint8_t {
    NO_EXTENSION = 0, // 没有扩展字段（最常见）
    HAS_EXTENSION = 1 // 后面有扩展字段，需解析更多数据
  };

  enum class FrameType {
    Keyframe = 1,
    Interframe,
    DisposableInterframe,
    Generatedkeyframe,
    Videoinfoframe
  };

  enum class CodecId {
    JPEG = 1,
    Sorenson,
    ScreenVideo,
    On2VP6,
    On2VP6WithAlphaChannel,
    ScreenVideoV2,
    AVC
  };

  enum class AVCPacketType : uint8_t {
    SEQUENCE_HEADER = 0, // 序列头（包含 AVCDecoderConfigurationRecord）
    NALU = 1,            // 正常的 NALU（压缩帧：I/P/B帧等）
  };

  AVTagDataBuilder() : size_(1024) { data_.reserve(size_); }
  ~AVTagDataBuilder() = default;

  AVTagDataBuilder &setCodecId(AVTagDataBuilder::CodecId codecid) {
    codecID_ = codecid;
    return *this;
  }

  AVTagDataBuilder &setFrameType(AVTagDataBuilder::FrameType frameType) {
    frameType_ = frameType;
    return *this;
  }

  AVTagDataBuilder &
  setAVCPacketType(AVTagDataBuilder::AVCPacketType packetType) {
    AVCPacketType_ = packetType;
    return *this;
  }

  AVTagDataBuilder &setCompositionTime(uint32_t compositionTime) {
    CompositionTime_[0] = compositionTime & 0xFF;
    CompositionTime_[1] = (compositionTime >> 8) & 0xFF;
    CompositionTime_[2] = (compositionTime >> 16) & 0xFF;
    return *this;
  }

  AVTagDataBuilder &setConfigurationVersion(uint8_t version) {
    configurationVersion_ = version;
    return *this;
  }

  AVTagDataBuilder &setAVCProfileIndication(uint8_t profile) {
    AVCProfileIndication_ = profile;
    return *this;
  }

  AVTagDataBuilder &setProfileCompatibility(uint8_t compatibility) {
    profileCompatibility_ = compatibility;
    return *this;
  }

  AVTagDataBuilder &setAVCLevelIndication(uint8_t level) {
    AVCLevelIndication_ = level;
    return *this;
  }

  AVTagDataBuilder &setLengthSizeMinusOne(uint8_t size) {
    lengthSizeMinusOne_ = size;
    return *this;
  }

  AVTagDataBuilder &setNumOfSps(uint16_t num) {
    uint8_t n = num & 0xFFFF;
    numOfSps_ = n | 0xE0;
    return *this;
  }

  AVTagDataBuilder &setSpsLength(uint16_t length) {
    spsLength_[0] = (length >> 8) & 0xff;
    spsLength_[1] = length & 0xff;
    return *this;
  }

  AVTagDataBuilder &setPpsLength(uint8_t length) {
    ppsLength_[0] = (length >> 8) & 0xff;
    ppsLength_[1] = length & 0xff;
    return *this;
  }

  AVTagDataBuilder &setPpsData(uint8_t *data, uint8_t length) {
    ppsData_ = data;
    ppsDataSize_ = length;
    return *this;
  }

  AVTagDataBuilder &setSpsData(uint8_t *data, uint8_t length) {
    spsData_ = data;
    spsDataSize_ = length;
    return *this;
  }

  AVTagDataBuilder &setNaluLength(uint32_t length) {
    naluLength_[0] = (length >> 24) & 0xff;
    naluLength_[1] = (length >> 16) & 0xff;
    naluLength_[2] = (length >> 8) & 0xff;
    naluLength_[3] = length & 0xff;
    return *this;
  }

  AVTagDataBuilder &setNaluData(uint8_t *data, uint32_t length) {
    naluData_ = data;
    naluDataSize_ = length;
    return *this;
  }

  AVTagDataBuilder &setSoundFormat(ESoundFormat soundFormat) {
    soundFormat_ = static_cast<uint32_t>(soundFormat);
    return *this;
  }

  AVTagDataBuilder &setSoundRate(ESoundRate soundRate) {
    soundRate_ = static_cast<uint16_t>(soundRate);
    return *this;
  }

  AVTagDataBuilder &setSoundSize(ESoundSize soundSize) {
    soundSize_ = static_cast<uint8_t>(soundSize);
    return *this;
  }

  AVTagDataBuilder &setSoundType(ESoundType soundType) {
    soundType_ = static_cast<uint8_t>(soundType);
    return *this;
  }

  AVTagDataBuilder &setAudioObjectType(EAudioObjectType audioObjectType) {
    audioObjectType_ = static_cast<uint8_t>(audioObjectType);
    return *this;
  }

  AVTagDataBuilder &
  setSamplingFreqIndex(ESamplingFrequencyIndex samplingFreqIndex) {
    samplingFreqIndex_ = static_cast<uint8_t>(samplingFreqIndex);
    return *this;
  }

  AVTagDataBuilder &
  setChannelConfiguration(EChannelConfiguration channelConfiguration) {
    channelConfiguration_ = static_cast<uint8_t>(channelConfiguration);
    return *this;
  }

  AVTagDataBuilder &setFrameLengthFlag(EFrameLengthFlag frameLengthFlag) {
    frameLengthFlag_ = static_cast<uint8_t>(frameLengthFlag);
    return *this;
  }

  AVTagDataBuilder &
  setDependsOnCoreCoder(EDependsOnCoreCoder dependsOnCoreCoder) {
    dependsOnCoreCoder_ = static_cast<uint8_t>(dependsOnCoreCoder);
    return *this;
  }

  AVTagDataBuilder &setExtensionFlag(EExtensionFlag extensionFlag) {
    extensionFlag_ = static_cast<uint8_t>(extensionFlag);
    return *this;
  }

  AVTagDataBuilder &setAudioData(uint8_t *data, uint32_t size) {
    audioData_ = data;
    audioDataSize_ = size;
    return *this;
  }

  void buildAudioSpecificConfig() {
    data_.emplace_back(soundFormat_ << 4 | soundRate_ << 2 | soundSize_ << 1 |
                       soundType_);
    uint16_t tag = 0;
    tag |= (audioObjectType_ & 0x1F) << 11;     // 5 bits
    tag |= (samplingFreqIndex_ & 0x0F) << 7;    // 4 bits
    tag |= (channelConfiguration_ & 0x0F) << 3; // 4 bits
    tag |= (frameLengthFlag_ & 0x01) << 2;      // 1 bit
    tag |= (dependsOnCoreCoder_ & 0x01) << 1;   // 1 bit
    tag |= (extensionFlag_ & 0x01);             // 1 bit
    data_.emplace_back(tag >> 8);               // 高8位
    data_.emplace_back(tag & 0xFF);             // 低8位
  }

  void buildAudioRawData() {
    data_.emplace_back(soundFormat_ << 4 | soundRate_ << 2 | soundSize_ << 1 |
                       soundType_);
    data_.emplace_back(AVCPacketType_);
    std::copy_n(audioData_, audioDataSize_, std::back_inserter(data_));
  }

  void buildVideoRawData() {
    data_.emplace_back(frameType_ << 4 | codecID_);
    data_.emplace_back(AVCPacketType_);
    std::copy_n(CompositionTime_, sizeof(CompositionTime_),
                std::back_inserter(data_));
    std::copy_n(naluLength_, sizeof(naluLength_), std::back_inserter(data_));
    std::copy_n(naluData_, naluDataSize_, std::back_inserter(data_));
  }

  void build() {
    data_.emplace_back(frameType_ << 4 | codecID_);
    data_.emplace_back(AVCPacketType_);
    std::copy_n(CompositionTime_, sizeof(CompositionTime_),
                std::back_inserter(data_));
    data_.emplace_back(configurationVersion_);
    data_.emplace_back(AVCProfileIndication_);
    data_.emplace_back(profileCompatibility_);
    data_.emplace_back(AVCLevelIndication_);
    data_.emplace_back(lengthSizeMinusOne_);
    data_.emplace_back(numOfSps_);
    std::copy_n(spsLength_, sizeof(spsLength_), std::back_inserter(data_));
    std::copy_n(spsData_, spsDataSize_, std::back_inserter(data_));
    data_.emplace_back(numOfpps_);
    std::copy_n(ppsLength_, sizeof(ppsLength_), std::back_inserter(data_));
    std::copy_n(ppsData_, ppsDataSize_, std::back_inserter(data_));
  }

  uint32_t size() const { return static_cast<uint32_t>(data_.size()); }
  uint8_t *data() { return data_.data(); }

private:
  std::vector<uint8_t> data_;
  // video tag data
  CodecId codecID_ : 4;     // 低4位
  FrameType frameType_ : 4; // 高4位
  AVCPacketType AVCPacketType_;
  uint8_t CompositionTime_[3];
  uint8_t configurationVersion_;
  uint8_t AVCProfileIndication_;
  uint8_t profileCompatibility_;
  uint8_t AVCLevelIndication_;
  uint8_t lengthSizeMinusOne_;
  uint8_t numOfSps_;
  uint8_t spsLength_[2];
  uint8_t *spsData_;
  uint32_t spsDataSize_;
  uint8_t numOfPps_;
  uint8_t ppsLength_[2];
  uint8_t *ppsData_;
  uint32_t ppsDataSize_;
  /* use for NALU*/
  uint8_t naluLength_[4];
  uint8_t *naluData_;
  uint32_t naluDataSize_;

  // audio tag header
  uint32_t soundFormat_;
  uint16_t soundRate_;
  uint8_t soundSize_;
  uint8_t soundType_;
  //  audio specific config
  uint8_t audioObjectType_;
  uint8_t samplingFreqIndex_;
  uint8_t channelConfiguration_;
  uint8_t frameLengthFlag_;
  uint8_t dependsOnCoreCoder_;
  uint8_t extensionFlag_;
  // audio raw data
  uint8_t *audioData_;
  uint32_t audioDataSize_;
};

#endif //_AV_TAG_DATA_BUILDER_H_
