/**
 *

 * */


#ifndef LEARNFFMPEG_SINGLEAUDIORECORDER_H
#define LEARNFFMPEG_SINGLEAUDIORECORDER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include "ThreadSafeQueue.h"
#include "thread"
#include "LogUtil.h"


using namespace std;

#define DEFAULT_SAMPLE_RATE    44100
#define DEFAULT_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO

class AudioFrame {
public:
    AudioFrame(uint8_t * data, int dataSize, bool hardCopy = true) {
        this->dataSize = dataSize;
        this->data = data;
        this->hardCopy = hardCopy;
        if(hardCopy) {
            this->data = static_cast<uint8_t *>(malloc(this->dataSize));
            memcpy(this->data, data, dataSize);
        }
    }

    ~AudioFrame() {
        if(hardCopy && this->data)
            free(this->data);
        this->data = nullptr;
    }

    uint8_t * data = nullptr;
    int dataSize = 0;
    bool hardCopy = true;
};

class SingleAudioRecorder {
public:
    SingleAudioRecorder(const char *outUrl, int sampleRate, int channelLayout, int sampleFormat);
    ~SingleAudioRecorder();
    //开始录制
    int StartRecord();
    //接收音频数据
    int OnFrame2Encode(AudioFrame *inputFrame);
    //停止录制
    int StopRecord();
    int getWorkAbleAudioQueueSize(){
        return m_frameQueue.Size();
    }

private:
    //编码循环
    static void StartAACEncoderThread(SingleAudioRecorder *context);
    //编码一帧的函数
    int EncodeFrame(AVFrame *pFrame);
private:
    ThreadSafeQueue<AudioFrame *> m_frameQueue;
    char m_outUrl[1024] = {0};
    int m_frameIndex = 0;
    int m_sampleRate;
    int m_channelLayout;
    int m_sampleFormat;
    AVPacket m_avPacket;
    AVFrame  *m_pFrame = nullptr;
    uint8_t *m_pFrameBuffer = nullptr;
    int m_frameBufferSize;
    AVCodec  *m_pCodec = nullptr;
    AVStream *m_pStream = nullptr;
    AVCodecContext *m_pCodecCtx = nullptr;
    AVFormatContext *m_pFormatCtx = nullptr;
    SwrContext *m_swrCtx = nullptr;
    thread *m_encodeThread = nullptr;
    volatile int m_exit = 0;
};


#endif //LEARNFFMPEG_SINGLEAUDIORECORDER_H
