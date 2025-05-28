#include "MsgQueue.h"

namespace Queue {
template <typename... MsgTypes> MsgQueue<MsgTypes...>::MsgQueue() {}
template <typename... MsgTypes> MsgQueue<MsgTypes...>::~MsgQueue() {}

template <typename... MsgTypes>
void MsgQueue<MsgTypes...>::sendMessage(
    const std::shared_ptr<std::variant<MsgTypes...>> &msg) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.emplace_front(msg);
  }
  cond_.notify_one();
}

template <typename... MsgTypes>
std::shared_ptr<std::variant<MsgTypes...>>
MsgQueue<MsgTypes...>::recvMessage() {
  std::unique_lock<std::mutex> lock(mutex_);
  cond_.wait(lock, [this] { return !queue_.empty(); });
  auto msg = queue_.back();
  queue_.pop_back();
  return msg;
}

} // namespace Queue
