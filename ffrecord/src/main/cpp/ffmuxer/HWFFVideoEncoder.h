//
// Created by manjialle on 2022/1/24.
//

#ifndef MYFFMPEGLEARN_HWFFVIDEOENCODER_H
#define MYFFMPEGLEARN_HWFFVIDEOENCODER_H

#include "VideoEncoder.h"
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaMuxer.h>

class HWFFVideoEncoder : public VideoEncoder {

private:

    AMediaCodec *media_codec_ = nullptr;
    AMediaFormat *media_format_ = nullptr;

    AVStream *mAvStream = nullptr;
    AVCodecContext *mCodecCtx = nullptr;

    volatile int64_t mNextPts = 0;
    AVFrame *mFrame = nullptr;

    //编码的数据包
    AVPacket *m_Packet = nullptr;

    void write(VideoFrame  *videoFrame);

    void receive();

protected:


public:
    HWFFVideoEncoder() {}

    ~HWFFVideoEncoder() {}

    int start(AVFormatContext *m_AVFormatContext, RecorderParam *param);

    void stop();

    void clear();

    int dealOneFrame();

    double getTimestamp() {
        return mNextPts * av_q2d(getTimeBase());
    }

    AVRational getTimeBase();
};

#endif //MYFFMPEGLEARN_HWFFVIDEOENCODER_H
