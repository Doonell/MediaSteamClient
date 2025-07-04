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

using FLVMessage = std::shared_ptr<Message::FLVAudioMessage>;
using FLVMetaMessage = std::shared_ptr<Message::FLVMetaMessage>;
using VideoSequenceMessage = std::shared_ptr<Message::VideoSequenceMessage>;
using H264RawMessage = std::shared_ptr<Message::H264RawMessage>;
using AudioSpecificConfigMessage =
    std::shared_ptr<Message::AudioSpecificConfigMessage>;
using AudioRawDataMessage = std::shared_ptr<Message::AudioRawDataMessage>;

namespace Middleware {

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

  template <typename MsgType>
  uint32_t subscribe(std::function<void(const MsgType &)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_[typeid(MsgType).name()] = [callback](const MsgVariant &msg) {
      callback(std::get<MsgType>(msg));
    };

    return 12; // 返回一个示例的订阅ID
  }

  void delegate(uint32_t subscriptionId) {
    std::thread([this, subscriptionId]() {
      while (true) {
        auto msg = recvMessage();
        if (msg) {
          std::visit(
              [this, subscriptionId](auto &&m) {
                using MsgType = std::decay_t<decltype(m)>;
                auto it = callbacks_.find(typeid(MsgType).name());
                if (it != callbacks_.end()) {
                  for (const auto &callback : it->second) {
                    callback(m);
                  }
                }
              },
              *msg);
        }
      }
    }).detach();
  }

  std::variant<MsgTypes...> recvMessage() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return !queue_.empty(); });
    auto msg = queue_.back();
    queue_.pop_back();
    return msg;
  }

private:
  std::deque<std::variant<MsgTypes...>> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::map<std::string /*message */,
           std::vector<std::function<void(const std::variant<MsgTypes...> &)>>>
      callbacks_;
};

} // namespace Middleware

#endif // _MSG_QUEUE_H_
