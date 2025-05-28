#include "IRTMPProtocol.h"
#include "Queue/MsgQueue.h"
#include "RTMPPusher.h"
#include "librtmp/rtmp.h"

namespace TransProtocol {

class RTMPPusher : public ITransPusher {
public:
  RTMPPusher(const std::shared_ptr<IConfigurationFacade> &config,
             MsgQueue<FLVMessage> &msgQueue)
      : config_(config), msgQueue_(msgQueue),
        rtmpProtocol_(std::make_unique<IRTMPProtocol>()) {}
  ~RTMPPusher() = default;

  void start() override;
  void stop() override;
  void send(const std::string &data) override;
  void recv(const std::string &data) override;

private:
  std::shared_ptr<IConfigurationFacade> config_;
  MsgQueue<FLVMessage> &msgQueue_;
  std::unique_ptr<IRTMPProtocol> rtmpProtocol_;
};

} // namespace TransProtocol
