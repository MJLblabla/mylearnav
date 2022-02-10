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
    void ParseH264SequenceHeader(uint8_t *in_buffer, uint32_t in_ui32_size, uint8_t **in_sps_buffer, int &in_sps_size,
                                 uint8_t **in_pps_buffer, int &in_pps_size);
    uint32_t FindStartCode(uint8_t *in_buffer, uint32_t in_ui32_buffer_size, uint32_t in_ui32_code,
                           uint32_t &out_ui32_processed_bytes);
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
