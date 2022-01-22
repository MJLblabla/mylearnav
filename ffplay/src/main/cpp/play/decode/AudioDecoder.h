//
// Created by manjialle on 2022/1/20.
//
#ifndef MYFFMPEGLEARN_AUDIODECODER_HX
#define MYFFMPEGLEARN_AUDIODECODER_HX


#include "PacketDecoder.h"

extern "C" {
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
}

// 音频编码采样率
static const int AUDIO_DST_SAMPLE_RATE = 44100;
// 音频编码通道数
static const int AUDIO_DST_CHANNEL_COUNTS = 2;
// 音频编码声道格式
static const uint64_t AUDIO_DST_CHANNEL_LAYOUT = AV_CH_LAYOUT_STEREO;
// 音频编码比特率
static const int AUDIO_DST_BIT_RATE = 64000;
// ACC音频一帧采样数
static const int ACC_NB_SAMPLES = 1024;

class AudioDecoder : public PacketDecoder {

private:
    double timeBase = 0;
    //解码器上下文
    AVCodecContext *m_AVCodecContext = nullptr;
    //解码器
    AVCodec *m_AVCodec = nullptr;

    //解码的帧
    AVFrame *m_Frame = nullptr;
    AudioRender *m_AudioRender = nullptr;

    const AVSampleFormat DST_SAMPLT_FORMAT = AV_SAMPLE_FMT_S16;

    //audio resample context
    SwrContext *m_SwrContext = nullptr;

    uint8_t *m_AudioOutBuffer = nullptr;

    //number of sample per channel
    int m_nbSamples = 0;

    //dst frame data size
    int m_DstFrameDataSze = 0;

    void onDecoderReady();

    static void decodeThreadProc(AudioDecoder *audioDecoder);

    void dealPackQueue();

public:
    AudioDecoder() {}

    ~AudioDecoder() {}

    static  long  GetVideoDecoderTimestampForAVSync(void *audioDecoder){
        return  (reinterpret_cast<AudioDecoder *> (audioDecoder) )-> getCurrentPosition();
    }

    void init();

    void unInit();

    int start(AVFormatContext *m_AVFormatContext);

    void pause();

    void resume();

    void stop();

    void clearCache();

    void UpdateTimeStamp();

    void AVSync();
    void setStreamIdx(int streamIdx) {
        m_StreamIdx = streamIdx;
    }

    int getStreamIdx() {
        return m_StreamIdx;
    }

    int getBufferSize() {
        return m_PacketQueue->GetSize();
    }

    long getCurrentPosition() {
        return m_CurTimeStamp;
    }

    void setMessageCallback(void *context, MessageCallback callback) {
        m_MsgContext = context;
        m_MsgCallback = callback;
    }

    void pushAVPacket(AVPacket *avPacket) {
        m_PacketQueue->PushPacket(avPacket);
    }
};


#endif //MYFFMPEGLEARN_AUDIODECODER_HX
