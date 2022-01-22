//
// Created by manjialle on 2022/1/21.
//

#ifndef MYFFMPEGLEARN_VIDEOHWDECODER_H
#define MYFFMPEGLEARN_VIDEOHWDECODER_H

#include "PacketDecoder.h"

class VideoHWDecoder : public VideoDecoder{
public:
    virtual  void init() = 0;

    virtual void unInit() =0;

    virtual void start() = 0;

    virtual  void pause() =0;

    virtual void stop()=0;

    virtual void clearCache()=0;



};
#endif //MYFFMPEGLEARN_VIDEOHWDECODER_H
