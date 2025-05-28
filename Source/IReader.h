
class IReader {
public:
    virtual ~IReader() = default;
    virtual std::vector<char> readAll() = 0;
};
