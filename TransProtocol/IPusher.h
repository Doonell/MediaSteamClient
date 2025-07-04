class ITransPusher {
public:
  virtual ~ITransPusher() = default;
  virtual void start() = 0;
  virtual void stop() = 0;
};
