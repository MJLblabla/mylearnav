//
// Created by manjialle on 2021/8/20.
//


#ifndef OPENGLCAMERA2_BYTEFLOWRENDERCONTEXT_H
#define OPENGLCAMERA2_BYTEFLOWRENDERCONTEXT_H

#include <cstdint>
#include <jni.h>
#include <SingleVideoRecorder.h>
#include "SingleAudioRecorder.h"
#include "MediaRecorder.h"

#define RECORDER_TYPE_SINGLE_VIDEO  0 //仅录制视频
#define RECORDER_TYPE_SINGLE_AUDIO  1 //仅录制音频
#define RECORDER_TYPE_AV            2 //同时录制音频和视频,打包成 MP4 文件

class MediaRecorderContext {
public:
    MediaRecorderContext();

    ~MediaRecorderContext();

    static void CreateContext(JNIEnv *env, jobject instance);

    static void DeleteContext(JNIEnv *env, jobject instance);

    static MediaRecorderContext *GetContext(JNIEnv *env, jobject instance);

    int StartRecord(int recorderType, const char *outUrl, int frameWidth, int frameHeight,
                    long videoBitRate, int fps);

    void OnAudioData(uint8_t *pData, int size);

    void OnVideoFrame(void *ctx, int format, uint8_t *pBuffer, int width, int height,int size);

    int StopRecord();


private:
    static jfieldID s_ContextHandle;

    static void StoreContext(JNIEnv *env, jobject instance, MediaRecorderContext *pContext);

    SingleVideoRecorder *m_pVideoRecorder = nullptr;
    SingleAudioRecorder *m_pAudioRecorder = nullptr;
    MediaRecorder *m_pAVRecorder = nullptr;
    mutex m_mutex;

};


#endif //OPENGLCAMERA2_BYTEFLOWRENDERCONTEXT_H
