//
// Created by manjialle on 2022/1/24.
//

#ifndef MYFFMPEGLEARN_VIDEOENCODER_H
#define MYFFMPEGLEARN_VIDEOENCODER_H

#include "Encoder.h"
#include "../common/ThreadSafeQueue.h"

class VideoEncoder : public Encoder{

protected:
    //视频帧队列
    ThreadSafeQueue<VideoFrame *>
            mVideoFrameQueue;

public:

    void pushImg(VideoFrame *img){
        mVideoFrameQueue.Push(img);
    }

};
#endif //MYFFMPEGLEARN_VIDEOENCODER_H
