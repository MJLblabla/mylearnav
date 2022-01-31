
#ifndef MYFFMPEGLEARN_HWAUDIOENCODER_H
#define MYFFMPEGLEARN_HWAUDIOENCODER_H

#include "HWEncoder.h"

class HWAudioEncoder : public HWEncoder {

private:
    //音频帧队列
    ThreadSafeQueue<AudioFrame *>
            m_AudioFrameQueue;
    int m_SamplesCount = 0;

    bool encodeFrame(void *data, int size, int64_t pts);

    void recvFrame(MP4Muxer *mMuxer);

public:
    HWAudioEncoder() {}

    ~HWAudioEncoder() {}

    void pushImg(AudioFrame *audioFrame) {
        m_AudioFrameQueue.Push(audioFrame);
    }

    int getQueueSize() {
        return m_AudioFrameQueue.Size();
    }

    long getTimestamp();

    int start(MP4Muxer *mMuxer, RecorderParam *param);

    void stop();

    void clear();

    void flush(MP4Muxer *mMuxer);

    int dealOneFrame(MP4Muxer *mMuxer);


};

#endif