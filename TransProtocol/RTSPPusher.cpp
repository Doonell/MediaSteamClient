// #include "RTSPPusher.h"
// #include "../Logger/Logger.h"
// #include "../Util/timeutil.h"

// extern "C" {
// #include "libavutil/opt.h"
// #include <libavformat/avformat.h>
// }

// namespace TransProtocol {

// static int decode_interrupt_cb(void *ctx) {
//   RtspPusher *rtsp_puser = (RtspPusher *)ctx;
//   if (rtsp_puser->IsTimeout()) {
//     LOG_WARN("timeout:%dms", rtsp_puser->GetTimeout());
//     return 1;
//   }
//   LOG_INFO("block time:%lld", rtsp_puser->GetBlockTime());
//   return 0;
// }

// RTSPPusher::RTSPPusher(std::string url, std::string rtsp_transport,
//                        int audio_frame_duration, int video_frame_duration,
//                        int timeout)
//     : url_(std::move(url)), rtsp_transport_(std::move(rtsp_transport)),
//       audio_frame_duration_(audio_frame_duration),
//       video_frame_duration_(video_frame_duration), timeout_(timeout) {}

// RTSPPusher::~RTSPPusher() {
//   // Clean up resources if needed
// }

// bool RTSPPusher::setUp() {
//   int ret = 0;
//   char str_error[512] = {0};
//   // 初始化网络库
//   ret = avformat_network_init();
//   if (ret < 0) {
//     LOG_ERROR("avformat_network_init failed:%s", str_error);
//     return false;
//   }
//   // 分配AVFormatContext
//   ret = avformat_alloc_output_context2(&fmt_ctx_, NULL, "rtsp",
//   url_.c_str()); if (ret < 0) {
//     LOG_ERROR("avformat_alloc_output_context2 failed:%s", av_err2str(ret));
//     return false;
//   }
//   ret = av_opt_set(fmt_ctx_->priv_data, "rtsp_transport",
//                    rtsp_transport_.c_str(), 0);
//   if (ret < 0) {
//     LOG_ERROR("av_opt_set failed:%s", av_err2str(ret));
//     return false;
//   }
//   fmt_ctx_->interrupt_callback.callback = decode_interrupt_cb; //
//   设置超时回调 fmt_ctx_->interrupt_callback.opaque = this; return true;
// }

// bool RTSPPusher::connect(const std::string &url) {
//   LOG_INFO("Connecting to RTSP server at {}", url);

//   if (!createAudioStream(url) || !createVideoStream(url)) {
//     LOG_ERROR("Failed to create audio or video stream");
//     return false;
//   }

//   ResetTimeout();
//   // 连接服务器
//   int ret = avformat_write_header(fmt_ctx_, NULL);
//   if (ret < 0) {
//     LOG_WARN("Failed to write header: {}", av_err2str(ret));
//     return false;
//   }
//   LOG_INFO("avformat_write_header done");
//   return true;
// }

// bool RTSPPusher::sendVideoFrame(const AVFrame *frame) {
//   if (!frame) {
//     LOG_ERROR("Received null frame, cannot send");
//     return false;
//   }
//   LOG_INFO("Sending frame with PTS: {}", frame->pts);
//   // Implement frame sending logic here
//   return true; // Return true if frame is sent successfully
// }

// int RTSPPusher::sendPacket(AVPacket *pkt, MediaType media_type) {
//   AVRational dst_time_base;
//   AVRational src_time_base = {1, 1000}; // 我们采集、编码 时间戳单位都是ms
//   if (E_VIDEO_TYPE == media_type) {
//     pkt->stream_index = video_index_;
//     dst_time_base = video_stream_->time_base; // 90khz
//   } else if (E_AUDIO_TYPE == media_type) {
//     pkt->stream_index = audio_index_;
//     dst_time_base = audio_stream_->time_base;
//   } else {
//     LogError("unknown mediatype:%d", media_type);
//     return -1;
//   }
//   pkt->pts = av_rescale_q(pkt->pts, src_time_base, dst_time_base);
//   pkt->duration = 0;
//   ResetTimeout();
//   int ret = av_write_frame(fmt_ctx_, pkt);
//   if (ret < 0) {
//     LOG_INFO("av_write_frame failed:%s", str_error); //
//     出错没有回调给PushWork return -1;
//   }
//   return 0;
// }

// bool RTSPPusher::sendAudioFrame(const AVFrame *frame) {

//   if (!frame) {
//     LOG_ERROR("Received null frame, cannot send");
//     return false;
//   }
//   LOG_INFO("Sending audio frame with PTS: {}", frame->pts);
//   // Implement frame sending logic here
//   return true; // Return true if frame is sent successfully
// }

// bool RTSPPusher::create_video_stream(const std::string &url) {
//   if (!fmt_ctx_) {
//     LogError("fmt_ctx is null");
//     return RET_FAIL;
//   }
//   if (!ctx) {
//     LogError("ctx is null");
//     return RET_FAIL;
//   }
//   // 添加视频流
//   AVStream *vs = avformat_new_stream(fmt_ctx_, NULL);
//   if (!vs) {
//     LogError("avformat_new_stream failed");
//     return RET_FAIL;
//   }
//   vs->codecpar->codec_tag = 0;
//   // 从编码器拷贝信息
//   avcodec_parameters_from_context(vs->codecpar, ctx);
//   video_ctx_ = (AVCodecContext *)ctx;
//   video_stream_ = vs;
//   video_index_ = vs->index; // 整个索引非常重要 fmt_ctx_根据index判别
//   音视频包 return RET_OK;
// }

// bool RTSPPusher::create_audio_stream(const std::string &url) {
//   LOG_INFO("Creating RTSP audio stream for URL: {}", url);

//   if (!fmt_ctx_) {
//     LOG_ERROR("fmt_ctx is null");
//     return false;
//   }

//   if (!ctx) {
//     LOG_ERROR("ctx is null");
//     return false;
//   }

//   AVStream *as = avformat_new_stream(fmt_ctx_, NULL);
//   if (!as) {
//     LOG_ERROR("avformat_new_stream failed");
//     return false;
//   }

//   as->codecpar->codec_tag = 0;
//   // 从编码器拷贝信息
//   avcodec_parameters_from_context(as->codecpar, ctx);
//   audio_ctx_ = (AVCodecContext *)ctx;
//   audio_stream_ = as;
//   audio_index_ = as->index; // 整个索引非常重要 fmt_ctx_根据index判别
//   音视频包 return true;
// }

// void RtspPusher::Loop() {
//   LogInfo("Loop into");
//   int ret = 0;
//   AVPacket *pkt = NULL;
//   MediaType media_type;
//   while (true) {
//     if (request_abort_) {
//       LogInfo("abort request");
//       break;
//     }
//     ret = queue_->PopWithTimeout(&pkt, media_type, 2000);
//     if (0 == ret) {
//       if (request_abort_) {
//         LogInfo("abort request");
//         av_packet_free(&pkt);
//         break;
//       }
//       switch (media_type) {
//       case E_VIDEO_TYPE:
//         ret = sendPacket(pkt, media_type);
//         if (ret < 0) {
//           LogError("send video Packet failed");
//         }
//         av_packet_free(&pkt);
//         break;
//       case E_AUDIO_TYPE:
//         ret = sendPacket(pkt, media_type);
//         if (ret < 0) {
//           LogError("send audio Packet failed");
//         }
//         av_packet_free(&pkt);
//         break;
//       default:
//         break;
//       }
//     }
//   }
//   RestTiemout();
//   ret = av_write_trailer(fmt_ctx_);
//   if (ret < 0) {
//     char str_error[512] = {0};
//     av_strerror(ret, str_error, sizeof(str_error) - 1);
//     LogError("av_write_trailer failed:%s", str_error);
//     return;
//   }
//   LogInfo("avformat_write_header ok");
// }

// int64_t RtspPusher::GetBlockTime() {
//   return Time::TimesUtil::GetTimeMillisecond() - pre_time_;
// }

// int RtspPusher::GetTimeout() { return timeout_; }

// void RtspPusher::ResetTimeout() {
//   pre_time_ = TimesUtil::GetTimeMillisecond(); // 重置为当前时间
// }

// bool RtspPusher::IsTimeout() {
//   if (TimesUtil::GetTimeMillisecond() - pre_time_ > timeout_) {
//     return true; // 超时
//   }
//   return false;
// }

// } // namespace TransProtocol