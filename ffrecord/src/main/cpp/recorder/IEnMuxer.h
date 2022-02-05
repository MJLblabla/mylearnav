

#ifndef OPENGLCAMERA2_BYTEFLOWIEnMuxer_H
#define OPENGLCAMERA2_BYTEFLOWIEnMuxer_H

#include "MessageCallback.h"
#include "thread"
using namespace std;
class IEnMuxer {

protected:
    //编码码器状态
    volatile int m_EncoderState = STATE_UNKNOWN;


    char m_OutUrl[1024] = {0};
    volatile bool m_Exit = false;
    //同步
    std::mutex m_Mutex;
    std::condition_variable m_Cond;

public:
    int getEncoderState() {
        return m_EncoderState;
    }

    virtual int start(const char *url, RecorderParam *param) = 0;

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void stop() = 0;

    virtual void init() = 0;

    virtual void unInit() = 0;

    //添加音频数据到音频队列
    virtual int onFrame2Encode(AudioFrame *inputFrame) = 0;

    //添加视频数据到视频队列
    virtual int onFrame2Encode(VideoFrame *inputFrame) = 0;
};
#endif