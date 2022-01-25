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


using namespace std;

typedef NativeImage VideoFrame;

struct RecorderParam {
    //video
    int frameWidth;
    int frameHeight;
    long videoBitRate;
    int fps;

    //audio
    int audioSampleRate;
    int channelLayout;
    int sampleFormat;
};

#define DEFAULT_SAMPLE_RATE    44100
#define DEFAULT_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO

class AudioFrame {
public:
    AudioFrame(uint8_t *data, int dataSize, bool hardCopy = true) {
        this->dataSize = dataSize;
        this->data = data;
        this->hardCopy = hardCopy;
        if (hardCopy) {
            this->data = static_cast<uint8_t *>(malloc(this->dataSize));
            memcpy(this->data, data, dataSize);
        }
    }

    ~AudioFrame() {
        if (hardCopy && this->data)
            free(this->data);
        this->data = nullptr;
    }

    uint8_t *data = nullptr;
    int dataSize = 0;
    bool hardCopy = true;
};

class Encoder {

protected:
    //封装格式上下文
    AVFormatContext *m_AVFormatContext = nullptr;
    RecorderParam m_RecorderParam = {0};
    volatile int64_t mNextPts = 0;
    volatile int m_EncodeEnd = 0;
    volatile bool m_Exit = false;

    virtual int WritePacket(AVFormatContext *fmt_ctx, AVRational *time_base, AVStream *st,
                            AVPacket *pkt) = 0;


//    int WritePacket(AVFormatContext *fmt_ctx, AVRational *time_base, AVStream *st,
//                    AVPacket *pkt){
//        /* rescale output packet timestamp values from codec to stream timebase */
//        av_packet_rescale_ts(pkt, *time_base, st->time_base);
//        pkt->stream_index = st->index;
//
//        /* Write the compressed frame to the media file. */
//        return av_interleaved_write_frame(fmt_ctx, pkt);
//    }
//
public:

    virtual double getTimestamp() = 0;

    virtual AVRational getTimeBase() = 0;

    virtual int start(AVFormatContext *formatCtx, RecorderParam *param) = 0;

    virtual void stop() = 0;

    virtual void clear() = 0;

    virtual int dealOneFrame() = 0;

};

#endif //MYFFMPEGLEARN_ENCODER_H
