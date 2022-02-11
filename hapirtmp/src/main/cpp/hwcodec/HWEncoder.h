#ifndef MYFFMPEGLEARN_HWENCODER_H
#define MYFFMPEGLEARN_HWENCODER_H

#define MEDIACODEC_BITRATE_MODE_CQ  0 //MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CQ
#define MEDIACODEC_BITRATE_MODE_VBR 1 //MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_VBR
#define MEDIACODEC_BITRATE_MODE_CBR 2 //MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR
#define MEDIACODEC_TIMEOUT_USEC 100000//us

#include <ImageDef.h>
#include "ThreadSafeQueue.h"
#include "thread"
#include "../recorder/MessageCallback.h"
#include "RTMPPush.h"
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaMuxer.h>

class HWEncoder {

protected:
    AMediaCodec *media_codec_ = nullptr;
    AMediaFormat *media_format_ = nullptr;

    volatile bool m_Exit = false;
    int64_t baseTime = 0;
    long startTime = 0;
    long position = 0;

public:

    virtual long getTimestamp() = 0;

    virtual int start(RTMPPush *mRTMPPush, RecorderParam *param) = 0;

    virtual void stop() = 0;

    virtual void clear() = 0;

    virtual int dealOneFrame(RTMPPush *mRTMPPush) = 0;
    virtual void flush(RTMPPush *mRTMPPush)=0;
    virtual bool isTrackAdded(){
        return true;
    }
};

#endif