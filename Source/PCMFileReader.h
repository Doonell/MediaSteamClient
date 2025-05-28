class PCMFileReader : public IReader{
public:
    PCMFileReader(const std::string& filePath);
    ~PCMFileReader();

    std::vector<char> readAll();
    // 你可以根据需要添加更多读取方法
private:
    std::string filePath_;
};
