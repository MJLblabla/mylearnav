//
// Created by manjialle on 2022/1/24.
//

#ifndef MYFFMPEGLEARN_SOFTAUDIOENCODER_H
#define MYFFMPEGLEARN_SOFTAUDIOENCODER_H

#include "AudioEncoder.h"

class SoftAudioEncoder : public AudioEncoder {

    AVStream *mAvStream = nullptr;
    AVCodecContext *mCodecCtx = nullptr;
    AVCodec *mAdioCodec = nullptr;

    int m_frameBufferSize;
    uint8_t *m_pFrameBuffer = nullptr;
    AVPacket m_avPacket;

    AVFrame *m_pFrame = nullptr;
    SwrContext *m_pSwrCtx = nullptr;



protected:


public:
    SoftAudioEncoder() {}

    ~SoftAudioEncoder() {}

    int start(AVFormatContext *formatCtx, RecorderParam *param);

    void stop();

    void clear();

    double getTimestamp() {
        return mNextPts * av_q2d(getTimeBase());
    }

    int dealOneFrame();

    AVRational getTimeBase();
};


#endif //MYFFMPEGLEARN_SOFTAUDIOENCODER_H
