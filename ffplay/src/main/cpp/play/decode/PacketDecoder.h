//
// Created by manjialle on 2022/1/20.
//

#ifndef MYFFMPEGLEARN_PACKETDECODER_H
#define MYFFMPEGLEARN_PACKETDECODER_H
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
#include <libavcodec/jni.h>
};

#include "MessageCallback.h"
#include "../../common/AVPacketQueue.h"

class PacketDecoder {

protected:
    int m_StreamIdx = -1;
    AVPacketQueue *m_PacketQueue = nullptr;
    void *m_MsgContext = nullptr;
    MessageCallback m_MsgCallback = nullptr;
    //解码器状态
    volatile int m_DecoderState = STATE_UNKNOWN;

    thread *m_DecodeThread = nullptr;
    //当前播放时间
    long m_CurTimeStamp = 0;
    std::mutex m_Mutex;
    std::condition_variable m_Cond;

    void *m_AVDecoderContext = nullptr;
    AVSyncCallback m_AVSyncCallback = nullptr;//用作音视频同步

public:
    virtual void init() = 0;

    virtual void unInit() = 0;

    virtual int start(AVFormatContext *m_AVFormatContext) = 0;

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void stop() = 0;

    virtual void clearCache() = 0;

    virtual void setStreamIdx(int streamIdx) = 0;

    virtual int getStreamIdx() = 0;

    virtual int getBufferSize() = 0;

    virtual void pushAVPacket(AVPacket *avPacket) = 0;

    virtual void UpdateTimeStamp() = 0;

    virtual long getCurrentPosition() = 0;

    virtual void setMessageCallback(void *context, MessageCallback callback) = 0;

    virtual void AVSync() = 0;

    //设置音视频同步的回调
    virtual void SetAVSyncCallback(void *context, AVSyncCallback callback) {
        m_AVDecoderContext = context;
        m_AVSyncCallback = callback;
    }

};

#endif //MYFFMPEGLEARN_PACKETDECODER_H
