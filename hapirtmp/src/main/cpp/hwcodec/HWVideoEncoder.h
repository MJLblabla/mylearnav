#ifndef MYFFMPEGLEARN_HWVIDEOENCODER_H
#define MYFFMPEGLEARN_HWVIDEOENCODER_H

#include "HWEncoder.h"

class HWVideoEncoder : public HWEncoder {

private:  //视频帧队列
    ThreadSafeQueue<VideoFrame *>
            mVideoFrameQueue;
    volatile int64_t outFrame_idx_ = 0;
    volatile int64_t inFrame_idx_ = 0;
    bool encodeFrame(void *data, int size, int64_t pts);

    void recvFrame(RTMPPush *mRTMPPush);


public:
    HWVideoEncoder() {}

    ~HWVideoEncoder() {}

    void pushImg(VideoFrame *img) {
        mVideoFrameQueue.Push(img);
    }

    int getQueueSize() {
        return mVideoFrameQueue.Size();
    }

    long getTimestamp();

    int start(RTMPPush *mRTMPPush, RecorderParam *param);

    void stop();

    void clear();

    void flush(RTMPPush *mRTMPPush);

    int dealOneFrame(RTMPPush *mRTMPPush);
};

#endif