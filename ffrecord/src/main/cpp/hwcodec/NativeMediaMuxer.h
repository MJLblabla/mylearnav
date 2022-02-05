
#include "../recorder/IEnMuxer.h"

#include "HWVideoEncoder.h"
#include "HWAudioEncoder.h"
#include "MP4Muxer.h"

class NativeMediaMuxer : public IEnMuxer {

protected:

    MP4Muxer *mp4Muxer = nullptr;
    HWVideoEncoder *mVideoEncoder = nullptr;
    HWAudioEncoder *mAudioEncoder = nullptr;
    //编码器线程
    thread *videoEncoderThread = nullptr;
    //编码器线程
    thread *audioEncoderThread = nullptr;

    static void startVideoMediaEncodeThread(NativeMediaMuxer *recorder);
    static void startAudioMediaEncodeThread(NativeMediaMuxer *recorder);
    void loopVideoEncoder();
    void loopAudioEncoder();

public:
    int start(const char *url, RecorderParam *param);

    void pause();

    void resume();

    void stop();

    void init();

    void unInit();

    //添加音频数据到音频队列
    int onFrame2Encode(AudioFrame *inputFrame);

    //添加视频数据到视频队列
    int onFrame2Encode(VideoFrame *inputFrame);
};