class YUVFileReader : public IReader{
public:
    YUVFileReader(const std::string& filePath);
    ~YUVFileReader();

    std::vector<char> readAll();
    // 你可以根据需要添加更多读取方法
private:
    std::string filePath_;
};