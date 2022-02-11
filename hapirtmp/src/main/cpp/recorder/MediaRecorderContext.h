//
// Created by manjialle on 2021/8/20.
//


#ifndef OPENGLCAMERA2_BYTEFLOWRENDERCONTEXT_H
#define OPENGLCAMERA2_BYTEFLOWRENDERCONTEXT_H

#include <cstdint>
#include <jni.h>


#include "../hwcodec/NativePusher.h"


class MediaRecorderContext {
public:
    MediaRecorderContext();

    ~MediaRecorderContext();

    static void CreateContext(JNIEnv *env, jobject instance);

    static void DeleteContext(JNIEnv *env, jobject instance);

    static MediaRecorderContext *GetContext(JNIEnv *env, jobject instance);

    int StartRecord(const char *outUrl, int frameWidth, int frameHeight,
                    int videoBitRate, int fps, int audioSampleRate,
                    int audioChannelCount,
                    int audioSampleFormat);

    void OnAudioData(uint8_t *pData, int size);

    void OnVideoFrame(int format, uint8_t *pBuffer, int width, int height, int size,
                      int pixelStride, int rowPadding);

    int StopRecord();

private:
    bool isStart = false;
    static jfieldID s_ContextHandle;

    static void StoreContext(JNIEnv *env, jobject instance, MediaRecorderContext *pContext);

    IPusher *mEnMuxer = nullptr;
    mutex m_mutex;
    RecorderParam mParam = {0};
    uint8_t *src_yuv = nullptr;
};


#endif //OPENGLCAMERA2_BYTEFLOWRENDERCONTEXT_H
