class ITransPusher {
public:
    virtual ~ITransPusher() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void send(const std::string& data) = 0;
    virtual void recv(const std::string& data) = 0;
};
