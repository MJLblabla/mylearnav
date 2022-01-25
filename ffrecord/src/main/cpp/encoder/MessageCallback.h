//
// Created by manjialle on 2022/1/22.
//

#ifndef MYFFMPEGLEARN_MESSAGECALLBACK_H
#define MYFFMPEGLEARN_MESSAGECALLBACK_H
#define MAX_PATH   2048
#define DELAY_THRESHOLD 100 //100ms

enum EecoderType{
    SOFT,
    HW,
};

enum EecoderState {
    STATE_UNKNOWN,
    STATE_DECODING,
    STATE_PAUSE,
    STATE_STOP,
    STATE_FINISH,
};

enum EecoderMsg {
    MSG_DECODER_INIT_ERROR,
    MSG_DECODER_READY,
    MSG_DECODER_DONE,
    MSG_REQUEST_RENDER,
    MSG_DECODING_TIME
};

typedef void (*MessageCallback)(void *, int, float);

typedef long (*AVSyncCallback)(void *);


#endif //MYFFMPEGLEARN_MESSAGECALLBACK_H
