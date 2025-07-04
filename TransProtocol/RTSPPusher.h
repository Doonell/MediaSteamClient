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
//   // Ĭ��23.2ms 44.1khz  1024*1000ms/44100=23.21995649ms
//   double audio_frame_duration_ = 23.21995649;
//   // 40ms ��Ƶ֡��Ϊ25��  �� 1000ms/25=40ms
//   double video_frame_duration_ = 40;
//   // ����ʱ
//   int timeout_;

//   PacketQueue *queue_ = NULL;

//   // �����������������
//   AVFormatContext *fmt_ctx_ = NULL;
//   // ��Ƶ������������
//   AVCodecContext *video_ctx_ = NULL;
//   // ��ƵƵ������������
//   AVCodecContext *audio_ctx_ = NULL;
//   // ���ɷ�
//   AVStream *video_stream_ = NULL;
//   int video_index_ = -1;
//   AVStream *audio_stream_ = NULL;
//   int audio_index_ = -1;

//   int64_t pre_time_ = 0; // ��¼����ffmpeg api֮ǰ��ʱ��
// };

// } // namespace TransProtocol
// #endif // _RTSP_PUSHER_H_