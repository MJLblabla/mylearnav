//
// Created by manjialle on 2022/1/24.
//

#ifndef MYFFMPEGLEARN_ENMUXER_H
#define MYFFMPEGLEARN_ENMUXER_H


#include "SoftVideoEncoder.h"
#include "SoftAudioEncoder.h"
#include "../recorder/IEnMuxer.h"
#include "HWFFVideoEncoder.h"


class EnMuxer : public IEnMuxer {

private:
    //封装格式上下文
    AVFormatContext *m_AVFormatContext = nullptr;
    //编码器线程
    thread *encoderThread = nullptr;
    VideoEncoder *mVideoEncoder;
    AudioEncoder *mAudioEncoder;
    int encoderType = 1;
    static void startMediaEncodeThread(EnMuxer *recorder);

    void loopEncoder();

public:

    int start(const char *url, RecorderParam *param);

    void pause();

    void resume();

    void stop();

    void init();

    void unInit();

    //添加音频数据到音频队列
    int onFrame2Encode(AudioFrame *inputFrame);

    //添加视频数据到视频队列
    int onFrame2Encode(VideoFrame *inputFrame);
};


#endif //MYFFMPEGLEARN_ENMUXER_H