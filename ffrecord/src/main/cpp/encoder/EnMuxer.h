//
// Created by manjialle on 2022/1/24.
//

#ifndef MYFFMPEGLEARN_ENMUXER_H
#define MYFFMPEGLEARN_ENMUXER_H

#include "MessageCallback.h"
#include "SoftVideoEncoder.h"
#include "SoftAudioEncoder.h"
#include "HWVideoEncoder.h"

class EnMuxer {

private:
    //封装格式上下文
    AVFormatContext *m_AVFormatContext = nullptr;
    //编码器线程
    thread *encoderThread = nullptr;
    //编码码器状态
    volatile int m_EncoderState = STATE_UNKNOWN;
    char m_OutUrl[1024] = {0};
    volatile bool m_Exit = false;
    //同步
    std::mutex m_Mutex;
    std::condition_variable m_Cond;

    VideoEncoder *mVideoEncoder;
    AudioEncoder *mAudioEncoder;

    static void startMediaEncodeThread(EnMuxer *recorder);

    void loopEncoder();

public:
    int getEncoderState() {
        return m_EncoderState;
    }

    void start(const char *url, RecorderParam *param);

    void pause();

    void resume();

    void stop();

    void init(EecoderType decoderType);

    void unInit();

    //添加音频数据到音频队列
    int onFrame2Encode(AudioFrame *inputFrame);

    //添加视频数据到视频队列
    int onFrame2Encode(VideoFrame *inputFrame);


};


#endif //MYFFMPEGLEARN_ENMUXER_H
