//
// Created by manjialle on 2022/1/24.
//

#ifndef MYFFMPEGLEARN_AUDIOENCODER_H
#define MYFFMPEGLEARN_AUDIOENCODER_H

#include "Encoder.h"
#include "../common/ThreadSafeQueue.h"

class AudioEncoder : public Encoder {
protected:
    //音频帧队列
    ThreadSafeQueue<AudioFrame *>
            m_AudioFrameQueue;
public:

    void pushImg(AudioFrame *audioFrame) {
        m_AudioFrameQueue.Push(audioFrame);
    }

    int getQueueSize(){
        return m_AudioFrameQueue.Size();
    }
};


#endif //MYFFMPEGLEARN_AUDIOENCODER_H
