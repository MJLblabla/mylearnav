//
// Created by manjialle on 2022/1/22.
//

#ifndef MYFFMPEGLEARN_VIDEOSOFTDECODER_H
#define MYFFMPEGLEARN_VIDEOSOFTDECODER_H


extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/jni.h>
};

#include "PacketDecoder.h"
#include "VideoDecoder.h"

class VideoSoftDecoder : public VideoDecoder {

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

    static void decodeThreadProc(VideoSoftDecoder *audioDecoder);

    void dealPackQueue();

public:
    VideoSoftDecoder(){};

    ~VideoSoftDecoder(){};

    void init();

    void unInit();

    int start(AVFormatContext *m_AVFormatContext);

    void UpdateTimeStamp();
    void AVSync();
    void pause();
    void resume();
    void stop();

    void clearCache();
};

#endif //MYFFMPEGLEARN_VIDEOSOFTDECODER_H
