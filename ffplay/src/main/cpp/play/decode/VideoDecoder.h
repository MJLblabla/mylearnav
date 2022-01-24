//
// Created by manjialle on 2022/1/20.
//

#ifndef MYFFMPEGLEARN_VIDEODECODER_HZ
#define MYFFMPEGLEARN_VIDEODECODER_HZ
extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/jni.h>
};

#include "PacketDecoder.h"
#include "../render/video/VideoRender.h"


class VideoDecoder : public PacketDecoder {

protected:
    double timeBase = 0;
    VideoRender *m_VideoRender = nullptr;
    //当前播放时间
    long m_CurTimeStamp = 0;

private:
    //解码器上下文
    AVCodecContext *m_AVCodecContext = nullptr;
    //解码器
    AVCodec *m_AVCodec = nullptr;

    //解码的帧
    AVFrame *m_Frame = nullptr;

    //number of sample per channel
    int m_nbSamples = 0;

    //dst frame data size
    int m_DstFrameDataSze = 0;


    const AVPixelFormat DST_PIXEL_FORMAT = AV_PIX_FMT_RGBA;

    int m_VideoWidth = 0;
    int m_VideoHeight = 0;

    int m_RenderWidth = 0;
    int m_RenderHeight = 0;

    AVFrame *m_RGBAFrame = nullptr;
    uint8_t *m_FrameBuffer = nullptr;

    SwsContext *m_SwsContext = nullptr;

    void onDecoderReady();

    static void decodeThreadProc(VideoDecoder *decoder);

    void dealPackQueue();

public:
    VideoDecoder() {};

    ~VideoDecoder() {};

    void init();

    void unInit();

    int start(AVFormatContext *m_AVFormatContext);

    void UpdateTimeStamp();

    void AVSync();

    void pause();

    void resume();

    void stop();

    void clearCache();

    void setVideoRender(VideoRender *videoRender) {
        m_VideoRender = videoRender;
    }

    void setStreamIdx(int streamIdx) {
        m_StreamIdx = streamIdx;
    }

    int getStreamIdx() {
        return m_StreamIdx;
    }

    void pushAVPacket(AVPacket *avPacket) {
        m_PacketQueue->PushPacket(avPacket);
    }

    int getBufferSize() {
        return m_PacketQueue->GetSize();
    }

    void setMessageCallback(void *context, MessageCallback callback) {
        m_MsgContext = context;
        m_MsgCallback = callback;
    }

    long getCurrentPosition() {
        return m_CurTimeStamp;
    }
};

#endif //MYFFMPEGLEARN_VIDEODECODER_HZ
