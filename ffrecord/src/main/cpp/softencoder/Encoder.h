//
// Created by manjialle on 2022/1/24.
//

#ifndef MYFFMPEGLEARN_ENCODER_H
#define MYFFMPEGLEARN_ENCODER_H

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <ImageDef.h>
#include "ThreadSafeQueue.h"
#include "thread"
#include "../recorder/MessageCallback.h"

using namespace std;


class Encoder {

protected:
    //封装格式上下文
    AVFormatContext *m_AVFormatContext = nullptr;
    RecorderParam m_RecorderParam = {0};
    volatile int64_t mNextPts = 0;
    volatile int m_EncodeEnd = 0;
    volatile bool m_Exit = false;


    int WritePacket(AVFormatContext *fmt_ctx, AVRational *time_base, AVStream *st,
                    AVPacket *pkt){
        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(pkt, *time_base, st->time_base);
        pkt->stream_index = st->index;
        PrintfPacket(fmt_ctx, pkt);
        /* Write the compressed frame to the media file. */
        return av_interleaved_write_frame(fmt_ctx, pkt);
    }


    void PrintfPacket(AVFormatContext *fmt_ctx, AVPacket *pkt) {
        AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

        LOGCATE("MediaRecorder::PrintfPacket pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
                av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
                av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
                av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
                pkt->stream_index);
    }

public:

    virtual double getTimestamp() = 0;

    virtual AVRational getTimeBase() = 0;

    virtual int start(AVFormatContext *formatCtx, RecorderParam *param) = 0;

    virtual void stop() = 0;

    virtual void clear() = 0;

    virtual int dealOneFrame() = 0;

};

#endif //MYFFMPEGLEARN_ENCODER_H
