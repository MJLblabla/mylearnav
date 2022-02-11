#ifndef OPENGLCAMERA2_RTMPPUSH_H
#define OPENGLCAMERA2_RTMPPUSH_H

#include <rtmp.h>
#include "ThreadSafeQueue.h"
#include "thread"
using namespace std;

typedef std::function<void(int)> ConnectCallBack;


class RTMPPush {

private:

    /**
   * rtmp 推流实例
   */
    RTMP *mRtmp = 0;
    /**
     * 推流链接
     */
    char mRtmpUrl[1024] = {0};
    /**
    * 推流时间
    */
    long mStartTime = 0;

    //音频帧队列
    ThreadSafeQueue<RTMPPacket *>
            rtmpPackQueue;

    bool  isConnect = false;
    volatile bool m_Exit = false;
    thread *pushThread = nullptr;
    static void startThread(RTMPPush *push);
    void loopPush();
    static void connect(const ConnectCallBack& callBack,RTMPPush *push);

public:
    RTMPPush();

    ~RTMPPush();

    void start(const char *url,ConnectCallBack callBack);

    void stop();

    void pushSpsPps(uint8_t *sps, int sps_len, uint8_t *pps, int pps_len);

    void pushAudioData(uint8_t *audio, int len, int type);

    void pushVideoData(uint8_t *video, int len, int type);

};

#endif