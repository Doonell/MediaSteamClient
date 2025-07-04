// #ifndef _RTSP_PUSHER_H_
// #define _RTSP_PUSHER_H_

// extern "C" {
// #include "libavcodec/avcodec.h"
// #include "libavformat/avformat.h"
// #include "libavformat/avio.h"
// #include "libavutil/opt.h"
// }
// #include <string>

// namespace TransProtocol {
// typedef enum media_type {
//   E_MEDIA_UNKNOWN = -1,
//   E_AUDIO_TYPE,
//   E_VIDEO_TYPE
// } MediaType;

// class RTSPPusher {
// public:
//   RTSPPusher(std::string url, std::string rtsp_transport,
//              int audio_frame_duration, int video_frame_duration, int
//              timeout);
//   ~RTSPPusher();

//   bool setUp();
//   bool connect(const std::string &url);
//   int sendPacket(AVPacket *pkt, MediaType media_type);

//   bool sendVideoFrame(const AVFrame *frame);
//   bool sendAudioFrame(const AVFrame *frame);
//   bool create_audio_stream(const std::string &url);
//   bool create_video_stream(const std::string &url);

//   int64_t GetBlockTime();
//   int GetTimeout();
//   void ResetTimeout();
//   bool IsTimeout();

// private:
//   std::string url_ = "";
//   std::string rtsp_transport_ = "";
//   // 默认23.2ms 44.1khz  1024*1000ms/44100=23.21995649ms
//   double audio_frame_duration_ = 23.21995649;
//   // 40ms 视频帧率为25的  ， 1000ms/25=40ms
//   double video_frame_duration_ = 40;
//   // 处理超时
//   int timeout_;

//   PacketQueue *queue_ = NULL;

//   // 整个输出流的上下文
//   AVFormatContext *fmt_ctx_ = NULL;
//   // 视频编码器上下文
//   AVCodecContext *video_ctx_ = NULL;
//   // 音频频编码器上下文
//   AVCodecContext *audio_ctx_ = NULL;
//   // 流成分
//   AVStream *video_stream_ = NULL;
//   int video_index_ = -1;
//   AVStream *audio_stream_ = NULL;
//   int audio_index_ = -1;

//   int64_t pre_time_ = 0; // 记录调用ffmpeg api之前的时间
// };

// } // namespace TransProtocol
// #endif // _RTSP_PUSHER_H_