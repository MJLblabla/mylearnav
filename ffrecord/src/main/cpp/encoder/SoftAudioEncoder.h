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

    int m_SamplesCount = 0;
    AVFrame *m_pFrame = nullptr;
    AVFrame *m_pTmpFrame = nullptr;
    SwsContext *m_pSwsCtx = nullptr;
    SwrContext *m_pSwrCtx = nullptr;

    AVFrame *AllocAudioFrame(AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate,
                             int nb_samples);

protected:
    int WritePacket(AVFormatContext *fmt_ctx, AVRational *time_base, AVStream *st,
                    AVPacket *pkt);

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
