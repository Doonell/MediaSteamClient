#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

#include "../Message/AudioMessage.h"
#include "../Message/VideoMessage.h"
#include < map>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

using FLVAudioMessage = std::shared_ptr<Message::FLVAudioMessage>;
using FLVMetaMessage = std::shared_ptr<Message::FLVMetaMessage>;
using VideoSequenceMessage = std::shared_ptr<Message::VideoSequenceMessage>;
using H264RawMessage = std::shared_ptr<Message::H264RawMessage>;
using AudioSpecificConfigMessage =
    std::shared_ptr<Message::AudioSpecificConfigMessage>;
using AudioRawDataMessage = std::shared_ptr<Message::AudioRawDataMessage>;

using AudioMessage = std::shared_ptr<Message::AudioMessage>;
using VideoMessage = std::shared_ptr<Message::VideoMessage>;
using MessageVariant =
    std::variant<FLVAudioMessage, FLVMetaMessage, VideoSequenceMessage,
                 H264RawMessage, AudioSpecificConfigMessage,
                 AudioRawDataMessage, AudioMessage, VideoMessage>;
namespace Middleware {

struct IReceiver {
  virtual void dispatch(const MessageVariant &) = 0;
  virtual ~IReceiver() = default;
};

template <typename T> struct scenario : virtual IReceiver {
  virtual ~scenario() = default;
  virtual void handle(const T &) = 0;
};

template <typename... Ts> struct BaseTrigger : scenario<Ts>... {
  virtual ~BaseTrigger() = default;

  void dispatch(const MessageVariant &msg) override {
    std::visit(
        [this](auto &&value) {
          using T = std::decay_t<decltype(value)>;
          if (auto *self = dynamic_cast<scenario<T> *>(this)) {
            self->handle(value);
          }
        },
        msg);
  }
};

template <typename... MsgTypes> class MsgQueue {
private:
  MsgQueue() = default;
  ~MsgQueue() = default;

public:
  MsgQueue(const MsgQueue &) = delete;
  MsgQueue &operator=(const MsgQueue &) = delete;

  static MsgQueue<MsgTypes...> &create() {
    static MsgQueue<MsgTypes...> msgQueue;
    return msgQueue;
  }

  template <typename T> void publish(const T &msg) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      queue_.emplace_back(msg);
    }
    cond_.notify_one();
  }

  void addReceiver(std::shared_ptr<IReceiver> tr) { callbacks_.push_back(tr); }

  void delegate() {
    std::thread([&]() {
      while (true) {
        auto msg = recvMessage();
        for (auto &receiver : callbacks_) {
          receiver->dispatch(msg);
        }
      }
    }).detach();
  }

  std::variant<MsgTypes...> recvMessage() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return !queue_.empty(); });
    auto msg = queue_.front();
    queue_.pop_front();
    return msg;
  }

private:
  std::deque<std::variant<MsgTypes...>> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::vector<std::shared_ptr<IReceiver>> callbacks_;
};

} // namespace Middleware

#endif // _MSG_QUEUE_H_
