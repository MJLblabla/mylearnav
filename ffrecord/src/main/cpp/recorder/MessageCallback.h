//
// Created by manjialle on 2022/1/22.
//

#ifndef MYFFMPEGLEARN_MESSAGECALLBACK_H
#define MYFFMPEGLEARN_MESSAGECALLBACK_H

#include <ImageDef.h>

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

typedef NativeImage VideoFrame;

struct RecorderParam {
    //video
    int frameWidth;
    int frameHeight;
    int videoBitRate;
    int fps;

    //audio
    int audioSampleRate;
    int channelLayout;
    int sampleFormat;
};

#define DEFAULT_SAMPLE_RATE    44100
#define DEFAULT_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO

class AudioFrame {
public:
    AudioFrame(uint8_t *data, int dataSize, bool hardCopy = true) {
        this->dataSize = dataSize;
        this->data = data;
        this->hardCopy = hardCopy;
        if (hardCopy) {
            this->data = static_cast<uint8_t *>(malloc(this->dataSize));
            memcpy(this->data, data, dataSize);
        }
    }

    ~AudioFrame() {
        if (hardCopy && this->data)
            free(this->data);
        this->data = nullptr;
    }

    uint8_t *data = nullptr;
    int dataSize = 0;
    bool hardCopy = true;
};

#endif //MYFFMPEGLEARN_MESSAGECALLBACK_H
