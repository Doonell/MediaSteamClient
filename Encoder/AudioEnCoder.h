class AudioEnCoder {
public:
    AudioEnCoder(const IConfiguration& codec, const IProtocolEntity& protocolEntity);
    ~AudioEnCoder();
    
    void encode(const std::string& data);
    void setBitRate(int bitRate);
};
