#ifndef _RTSP_PUSHER_H_
#define _RTSP_PUSHER_H_

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/opt.h"
}
#include "../Encoder/AACEncoder.h"
#include "../Encoder/AudioS16Resampler.h"
#include "../Encoder/H264Encoder.h"
#include "../Message/VideoMessage.h"
#include "../Middleware/MsgQueue.h"
#include "../Util/TimeHelper.h"
#include <string>

#include "../Logger/Logger.h"
#include "../Util/timeutil.h"
#include "RTSPPusher.h"

using namespace TimeHelper;
using namespace Time;

namespace TransProtocol {
typedef enum media_type {
  E_MEDIA_UNKNOWN = -1,
  E_AUDIO_TYPE,
  E_VIDEO_TYPE
} MediaType;
template <typename Container> class RTSPPusher;
using RTSPPusherType = RTSPPusher<
    Middleware::MsgQueue<FLVAudioMessage, FLVMetaMessage, VideoSequenceMessage,
                         H264RawMessage, AudioSpecificConfigMessage,
                         AudioRawDataMessage, VideoMessage, AudioMessage>>;

template <typename Container>
class RTSPPusher : public Middleware::BaseTrigger<AudioMessage, VideoMessage>,
                   public std::enable_shared_from_this<RTSPPusher<Container>> {
public:
  RTSPPusher(Container &msgQueue,
             const std::shared_ptr<Encoder::AACEncoder> &audioEncoder,
             const std::shared_ptr<Encoder::IVideoEncoder> &videoEncoder,
             const std::shared_ptr<Encoder::AudioS16Resampler> &audioResampler,
             std::string url = "rtsp://192.168.133.129/live/livestream/sub",
             std::string rtsp_transport = "udp")
      : msgQueue_(msgQueue), audioEncoder_(audioEncoder),
        videoEncoder_(videoEncoder), audioResampler_(audioResampler),
        url_(std::move(url)), rtsp_transport_(std::move(rtsp_transport)) {}

  ~RTSPPusher() {
    if (fmt_ctx_) {
      avformat_free_context(fmt_ctx_);
      fmt_ctx_ = nullptr;
    }
  }

  bool start() {
    int ret = 0;
    // 初始化网络库
    ret = avformat_network_init();
    if (ret < 0) {
      LOG_ERROR("avformat_network_init failed:%s", av_err2str(ret));
      return false;
    }
    // 分配AVFormatContext
    ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, "rtsp",
                                         url_.c_str());
    if (ret < 0) {
      LOG_ERROR("avformat_alloc_output_context2 failed:%s", av_err2str(ret));
      return false;
    }

    ret = av_opt_set(fmt_ctx_->priv_data, "rtsp_transport",
                     rtsp_transport_.c_str(), 0);
    if (ret < 0) {
      LOG_ERROR("av_opt_set failed:%s", av_err2str(ret));
      return false;
    }

    fmt_ctx_->interrupt_callback.callback = decode_interrupt_cb;
    fmt_ctx_->interrupt_callback.opaque = this;
    if (videoEncoder_ &&
        !create_video_stream(videoEncoder_->getCodecContext())) {
      LOG_ERROR("Failed to create video stream");
      return false;
    }
    if (audioEncoder_ &&
        !create_audio_stream(audioEncoder_->getCodecContext())) {
      LOG_ERROR("Failed to create audio stream");
      return false;
    }
    if (!connect()) {
      LOG_ERROR("Failed to connect to RTSP server at {}", url_);
      return false;
    }

    msgQueue_.addReceiver(
        std::static_pointer_cast<Middleware::IReceiver>(shared_from_this()));
    msgQueue_.delegate();

    return true;
  }

  bool connect() {
    std::cout << "Connecting to RTSP server at " << url_ << std::endl;
    if (!audio_stream_ && !video_stream_) {
      return false;
    }
    ResetTimeout();
    // 连接服务器
    int ret = avformat_write_header(fmt_ctx_, nullptr);
    if (ret < 0) {
      char str_error[512] = {0};
      av_strerror(ret, str_error, sizeof(str_error) - 1);
      std::cout << "Failed to write header: " << str_error << std::endl;
      return false;
    }
    std::cout << "avformat_write_header done" << std::endl;
    return true;
  }

  void sendVideoPacket(uint8_t *yuv, int size) {
    std::function<void(AVPacket &)> handleH264Callback = [&](AVPacket &packet) {
      if (packet.size > 0) {
        auto videoMessage = std::make_shared<Message::VideoMessage>(packet);
        msgQueue_.publish(videoMessage);
      }
    };
    videoEncoder_->encode(yuv, handleH264Callback);
  }

  void sendRawVideoPacket(AVPacket &packet) {
    if (packet.size > 0) {
      auto videoMessage = std::make_shared<Message::VideoMessage>(packet);
      msgQueue_.publish(videoMessage);
    }
  }

  void sendAudioPacket(uint8_t *pcm, int size) {
    auto ret = audioResampler_->sendFrame(pcm, size);
    if (ret < 0) {
      LOG_INFO("sendFrame failed ");
      return;
    }

    std::vector<std::shared_ptr<AVFrame>> resampled_frames;
    ret = audioResampler_->receiveFrame(resampled_frames,
                                        audioEncoder_->GetFrameSampleSize());
    if (!ret) {
      LOG_INFO("receiveFrame ret:%d\n", ret);
      return;
    }

    for (int i = 0; i < resampled_frames.size(); i++) {
      int64_t pts = (int64_t)AVPublishTime::getInstance()->get_audio_pts();
      resampled_frames[i].get()->pts = pts;
      audioEncoder_->encode(resampled_frames[i].get(), [&](AVPacket &packet) {
        if (packet.size > 0) {
          auto rawData = std::make_shared<Message::AudioMessage>(packet);
          msgQueue_.publish(rawData);
        }
      });
    }
  }

  void scenario<AudioMessage>::handle(const AudioMessage &t) override {
    // std::cout << "Received AudioMessage with data size: " << t->data->size
    //           << std::endl;
    sendPacket(t->data.get(), E_AUDIO_TYPE);
  }

  void scenario<VideoMessage>::handle(const VideoMessage &t) override {
    // std::cout << "Received VideoMessage with data size: " << t->data->size
    //           << std::endl;
    sendPacket(t->data.get(), E_VIDEO_TYPE);
  }

  bool sendVideoFrame(const AVFrame *frame) {
    if (!frame) {
      LOG_ERROR("Received null frame, cannot send");
      return false;
    }
    LOG_INFO("Sending frame with PTS: {}", frame->pts);
    // Implement frame sending logic here
    return true; // Return true if frame is sent successfully
  }

  int sendPacket(AVPacket *pkt, MediaType media_type) {
    AVRational dst_time_base;
    AVRational src_time_base = {1, 1000}; // 我们采集、编码 时间戳单位都是ms
    if (E_VIDEO_TYPE == media_type) {
      pkt->stream_index = video_index_;
      dst_time_base = video_stream_->time_base; // 90khz
    } else if (E_AUDIO_TYPE == media_type) {
      pkt->stream_index = audio_index_;
      dst_time_base = audio_stream_->time_base;
    } else {
      LOG_INFO("unknown mediatype:%d", media_type);
      return -1;
    }
    pkt->pts = av_rescale_q(pkt->pts, src_time_base, dst_time_base);
    pkt->duration = 0;
    ResetTimeout();
    if (current_time_ == 0 && previous_time_ == 0) {
      previous_time_ = TimesUtil::getTimeMillisecond();
    }
    current_time_ = TimesUtil::getTimeMillisecond();
    if (current_time_ - previous_time_ > 1000) {
      previous_time_ = current_time_;
      std::cout << "Cumulative data size in last second: "
                << cumulative_data_size_ * 8 / 1024 << " kbps" << std::endl;
      cumulative_data_size_ = 0;      // 重置累计数据大小
      previous_time_ = current_time_; // 重置上一帧时间
    }
    cumulative_data_size_ += pkt->size;
    int ret = av_write_frame(fmt_ctx_, pkt);
    if (ret < 0) {
      LOG_INFO("av_write_frame failed:%s",
               str_error); // 出错没有回调给PushWork
      return -1;
    }
    return 0;
  }

  bool sendAudioFrame(const AVFrame *frame) {
    if (!frame) {
      LOG_ERROR("Received null frame, cannot send");
      return false;
    }
    LOG_INFO("Sending audio frame with PTS: {}", frame->pts);
    // Implement frame sending logic here
    return true; // Return true if frame is sent successfully
  }

  bool create_video_stream(const AVCodecContext *ctx) {
    if (!fmt_ctx_) {
      LOG_ERROR("fmt_ctx is null");
      return false;
    }
    if (!ctx) {
      LOG_ERROR("ctx is null");
      return false;
    }
    // 添加视频流
    AVStream *vs = avformat_new_stream(fmt_ctx_, nullptr);
    if (!vs) {
      LOG_ERROR("avformat_new_stream failed");
      return false;
    }
    vs->codecpar->codec_tag = 0;
    // 从编码器拷贝信息
    avcodec_parameters_from_context(vs->codecpar, ctx);
    video_ctx_ = (AVCodecContext *)ctx;
    video_stream_ = vs;
    video_index_ = vs->index; // 整个索引非常重要 fmt_ctx_根据index判别音视频包
    std::cout << "Created video stream with index: " << video_index_
              << std::endl;
    return true;
  }

  bool create_audio_stream(const AVCodecContext *ctx) {
    if (!fmt_ctx_) {
      LOG_ERROR("fmt_ctx is null");
      return false;
    }

    if (!ctx) {
      LOG_ERROR("ctx is null");
      return false;
    }

    AVStream *as = avformat_new_stream(fmt_ctx_, nullptr);
    if (!as) {
      LOG_ERROR("avformat_new_stream failed");
      return false;
    }

    as->codecpar->codec_tag = 0;
    // 从编码器拷贝信息
    avcodec_parameters_from_context(as->codecpar, ctx);
    audio_ctx_ = (AVCodecContext *)ctx;
    audio_stream_ = as;
    // 整个索引非常重要 fmt_ctx_根据index判别音视频包
    audio_index_ = as->index;
    std::cout << "Created audio stream with index: " << audio_index_
              << std::endl;
    return true;
  }

  int64_t GetBlockTime() {
    return Time::TimesUtil::getTimeMillisecond() - pre_time_;
  }

  int GetTimeout() { return timeout_; }

  void ResetTimeout() {
    pre_time_ = TimesUtil::getTimeMillisecond(); // 重置为当前时间
  }

  bool IsTimeout() {
    if (TimesUtil::getTimeMillisecond() - pre_time_ > timeout_) {
      return true; // 超时
    }
    return false;
  }

private:
  Container &msgQueue_;
  std::shared_ptr<Encoder::AACEncoder> audioEncoder_;
  std::shared_ptr<Encoder::IVideoEncoder> videoEncoder_;
  std::shared_ptr<Encoder::AudioS16Resampler> audioResampler_;
  std::string url_ = "";
  std::string rtsp_transport_ = "rtsp_transport";

  double audio_frame_duration_ = 23.21995649;
  double video_frame_duration_ = 40;
  int timeout_ = 5000;

  AVFormatContext *fmt_ctx_ = nullptr;
  AVCodecContext *video_ctx_ = nullptr;
  AVCodecContext *audio_ctx_ = nullptr;
  AVStream *video_stream_ = nullptr;
  int video_index_ = -1;
  AVStream *audio_stream_ = nullptr;
  int audio_index_ = -1;

  int64_t pre_time_ = 0;              // 记录调用ffmpeg api之前的时间
  uint64_t cumulative_data_size_ = 0; // 累计发送数据大小
  uint64_t current_time_ = 0;         // 当前时间
  uint64_t previous_time_ = 0;        // 上一帧时间
};

static int decode_interrupt_cb(void *ctx) {
  RTSPPusherType *rtsp_pusher = (RTSPPusherType *)ctx;
  if (rtsp_pusher->IsTimeout()) {
    LOG_WARN("timeout:%dms", rtsp_pusher->GetTimeout());
    return 1;
  }
  LOG_INFO("block time:%lld", rtsp_pusher->GetBlockTime());
  return 0;
}

} // namespace TransProtocol
#endif // _RTSP_PUSHER_H_