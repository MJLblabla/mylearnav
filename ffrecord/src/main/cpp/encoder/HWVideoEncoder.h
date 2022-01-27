//
// Created by manjialle on 2022/1/25.
//

#ifndef MYFFMPEGLEARN_HWVIDEOENCODER_H
#define MYFFMPEGLEARN_HWVIDEOENCODER_H

#include "VideoEncoder.h"
#include "media/NdkMediaCodec.h"

class HWVideoEncoder : public VideoEncoder {

private:
    AVStream *mAvStream = nullptr;
    AMediaCodec *pMediaCodec;
    AMediaFormat *format;

    //编码的数据包
    AVPacket *m_Packet = nullptr;


protected:
    int WritePacket(AVFormatContext *fmt_ctx, AVRational *time_base, AVStream *st,
                    AVPacket *pkt);

public:
    HWVideoEncoder() {}

    ~HWVideoEncoder() {}

    int start(AVFormatContext *m_AVFormatContext, RecorderParam *param);

    void stop();

    void clear();

    int dealOneFrame();

    double getTimestamp() {
        return mNextPts * av_q2d(getTimeBase());
    }

    AVRational getTimeBase();
};

#endif //MYFFMPEGLEARN_HWVIDEOENCODER_H
