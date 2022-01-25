//
// Created by manjialle on 2022/1/24.
//

#ifndef MYFFMPEGLEARN_SOFTVIDEOENCODER_H
#define MYFFMPEGLEARN_SOFTVIDEOENCODER_H

#include "VideoEncoder.h"

class SoftVideoEncoder : public VideoEncoder {

private:
    AVStream *mAvStream = nullptr;
    AVCodecContext *mCodecCtx = nullptr;
    AVCodec *mVideoCodec = nullptr;
    volatile int64_t mNextPts = 0;
    AVFrame *mFrame = nullptr;

    //编码的数据包
    AVPacket *m_Packet = nullptr;

    AVFrame *AllocVideoFrame(AVPixelFormat pix_fmt, int width, int height);


public:
    SoftVideoEncoder() {}

    ~SoftVideoEncoder() {}

    int start(AVFormatContext *m_AVFormatContext, RecorderParam *param);

    void stop();
    void clear();
    int dealOneFrame();

    AVRational getTimeBase();
};

#endif //MYFFMPEGLEARN_SOFTVIDEOENCODER_H
