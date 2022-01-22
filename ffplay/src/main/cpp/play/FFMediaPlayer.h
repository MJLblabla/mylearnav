/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#ifndef LEARNFFMPEG_FFMEDIAPLAYER_H
#define LEARNFFMPEG_FFMEDIAPLAYER_H

#include <jni.h>
#include "decode/DeMuxer.h"

#define JAVA_PLAYER_EVENT_CALLBACK_API_NAME "playerEventCallback"
#define MEDIA_PARAM_VIDEO_WIDTH         0x0001
#define MEDIA_PARAM_VIDEO_HEIGHT        0x0002
#define MEDIA_PARAM_VIDEO_DURATION      0x0003

#define MEDIA_PARAM_ASSET_MANAGER       0x0020

class FFMediaPlayer {
public:

    FFMediaPlayer() {};

    ~FFMediaPlayer() {};

    void Init(JNIEnv *jniEnv, jobject obj);

    void UnInit();

    void setVideoPlayerRender(VideoRender *videoRender);

    void Play( char *url);

    void Pause();

    void Stop();

    void SeekToPosition(float position);

    long GetMediaParams(int paramType);

private:

    DeMuxer *muxer =  nullptr;
    static void PostMessage(void *context, int msgType, float msgCode);

    JNIEnv *GetJNIEnv(bool *isAttach);

    jobject GetJavaObj();

    JavaVM *GetJavaVM();

    JavaVM *m_JavaVM = nullptr;
    jobject m_JavaObj = nullptr;
};


#endif //LEARNFFMPEG_FFMEDIAPLAYER_H
