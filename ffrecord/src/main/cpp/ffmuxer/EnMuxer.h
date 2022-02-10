//
// Created by manjialle on 2022/1/24.
//

#ifndef MYFFMPEGLEARN_ENMUXER_H
#define MYFFMPEGLEARN_ENMUXER_H


#include "SoftVideoEncoder.h"
#include "SoftAudioEncoder.h"
#include "../recorder/IEnMuxer.h"
//#include "HWVideoEncoder.h"

class EnMuxer : public IEnMuxer {

private:
    //封装格式上下文
    AVFormatContext *m_AVFormatContext = nullptr;
    //编码器线程
    VideoEncoder *mVideoEncoder;
    AudioEncoder *mAudioEncoder;
    //编码器线程
    thread *videoEncoderThread = nullptr;
    //编码器线程
    thread *audioEncoderThread = nullptr;

    static void startVideoMediaEncodeThread(EnMuxer *recorder);
    static void startAudioMediaEncodeThread(EnMuxer *recorder);
    void loopVideoEncoder();
    void loopAudioEncoder();

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
