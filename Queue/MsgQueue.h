#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

#include "AudioMessage.h"
#include "VideoMessage.h"
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <variant>

using MsgVariant =
    std::variant<FLVMessage, FLVMetaMessage, VideoSequenceMessage,
                 H264RawMessage, AudioSpecificConfigMessage,
                 AudioRawDataMessage>;

namespace Queue {

template <typename... MsgTypes> class MsgQueue {
public:
  MsgQueue();
  ~MsgQueue();

  void sendMessage(const std::shared_ptr<std::variant<MsgTypes...>> &msg);
  std::shared_ptr<std::variant<MsgTypes...>> recvMessage();

private:
  std::deque<MsgVariant> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;
};

} // namespace Queue

#endif // _MSG_QUEUE_H_
