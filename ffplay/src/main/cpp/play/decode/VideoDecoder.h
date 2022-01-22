//
// Created by manjialle on 2022/1/20.
//

#ifndef MYFFMPEGLEARN_VIDEODECODER_HZ
#define MYFFMPEGLEARN_VIDEODECODER_HZ


#include "PacketDecoder.h"
#include "../render/video/VideoRender.h"


class VideoDecoder : public PacketDecoder {

protected:
    double timeBase = 0;
    VideoRender *m_VideoRender = nullptr;
    //当前播放时间
    long m_CurTimeStamp = 0;
public:
    VideoDecoder(){}
    ~VideoDecoder(){}
    virtual  void init() = 0;

    virtual void unInit() =0;

    virtual int start( AVFormatContext *m_AVFormatContext ) = 0;

    virtual  void pause() =0;
    virtual void resume()=0;
    virtual void stop()=0;
    virtual void AVSync()=0;
    virtual void clearCache()=0;

    void setVideoRender(VideoRender *videoRender) {
        m_VideoRender = videoRender;
    }
    void setStreamIdx(int streamIdx){
        m_StreamIdx = streamIdx;
    }
    int getStreamIdx(){
        return m_StreamIdx;
    }
    void pushAVPacket(AVPacket *avPacket){
        m_PacketQueue->PushPacket(avPacket);
    }
    int  getBufferSize(){
        return m_PacketQueue->GetSize();
    }
    void setMessageCallback(void *context, MessageCallback callback) {
        m_MsgContext = context;
        m_MsgCallback = callback;
    }
    long getCurrentPosition(){
        return m_CurTimeStamp;
    }
};

#endif //MYFFMPEGLEARN_VIDEODECODER_HZ
