#ifndef LEARNFFMPEG_DECODER_HS
#define LEARNFFMPEG_DECODER_HS


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
#include <libavcodec/jni.h>
};

#include <thread>
#include "../../common/AVPacketQueue.h"
#include "../render/audio/AudioRender.h"
#include "../render/video/VideoRender.h"
#include "AudioDecoder.h"
#include "VideoDecoder.h"
#include "MessageCallback.h"
#include "VideoSoftDecoder.h"



class DeMuxer {

private:
    //封装格式上下文
    AVFormatContext *m_AVFormatContext = nullptr;
    thread *decoderThread = nullptr;
    AudioDecoder *m_AudioDecoder = nullptr;
    VideoDecoder *m_VideoDecoder = nullptr;
    //解码器状态
    volatile int  m_DecoderState = STATE_UNKNOWN;

    std::mutex               m_Mutex;
    std::condition_variable  m_Cond;
    //文件地址
    char       m_Url[MAX_PATH] = {0};
    //seek position
    volatile float      m_SeekPosition = 0;
    volatile bool       m_SeekSuccess = false;

    //总时长 ms
    long             m_Duration = 0;

    static  void doAVDecoding(DeMuxer *deMuxer);
    void decodingLoop();
    int decodeOnePacket();

public:

    void start(const char *url);

    void pause();
    void resume();

    void stop();

    void init(DecoderType decoderType );

    void unInit();

    void seekToPosition(float position);

    long getCurrentPosition();

    void setMessageCallback(void *context, MessageCallback callback);

    void setVideoRender(VideoRender *videoRender) {
        m_VideoDecoder->setVideoRender(videoRender);
    }

};

#endif //LEARNFFMPEG_DECODER_HS