
#include "FFMediaPlayer.h"


void FFMediaPlayer::Init(JNIEnv *jniEnv, jobject obj) {
    jniEnv->GetJavaVM(&m_JavaVM);
    m_JavaObj = jniEnv->NewGlobalRef(obj);
    av_jni_set_java_vm(m_JavaVM, NULL);
    muxer = new DeMuxer();
    muxer->init(SOFT);
    muxer->setMessageCallback(this, PostMessage);
}

void FFMediaPlayer::setVideoPlayerRender(VideoRender *videoRender) {
    muxer->setVideoRender(videoRender);
}

void FFMediaPlayer::UnInit() {
    muxer->unInit();
    delete muxer;
    muxer = nullptr;

    bool isAttach = false;
    GetJNIEnv(&isAttach)->DeleteGlobalRef(m_JavaObj);
    if (isAttach)
        GetJavaVM()->DetachCurrentThread();

}


void FFMediaPlayer::Play( char *url) {
    LOGCATE("FFMediaPlayer::Play");
    muxer->start(url);
}

void FFMediaPlayer::Pause() {
    LOGCATE("FFMediaPlayer::Pause");
    muxer->pause();
}

void FFMediaPlayer::Stop() {
    LOGCATE("FFMediaPlayer::Stop");
    muxer->stop();
}

void FFMediaPlayer::SeekToPosition(float position) {
    LOGCATE("FFMediaPlayer::SeekToPosition position=%f", position);
    muxer->seekToPosition(position);
}

long FFMediaPlayer::GetMediaParams(int paramType) {
    LOGCATE("FFMediaPlayer::GetMediaParams paramType=%d", paramType);
    long value = 0;
//    switch (paramType) {
//        case MEDIA_PARAM_VIDEO_WIDTH:
//            value = muxer != nullptr ? muxer->GetVideoWidth() : 0;
//            break;
//        case MEDIA_PARAM_VIDEO_HEIGHT:
//            value = m_VideoDecoder != nullptr ? m_VideoDecoder->GetVideoHeight() : 0;
//            break;
//        case MEDIA_PARAM_VIDEO_DURATION:
//            value = m_VideoDecoder != nullptr ? m_VideoDecoder->GetDuration() : 0;
//            break;
//    }
    return value;
}

JNIEnv *FFMediaPlayer::GetJNIEnv(bool *isAttach) {
    JNIEnv *env;
    int status;
    if (nullptr == m_JavaVM) {
        LOGCATE("FFMediaPlayer::GetJNIEnv m_JavaVM == nullptr");
        return nullptr;
    }
    *isAttach = false;
    status = m_JavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
    if (status != JNI_OK) {
        status = m_JavaVM->AttachCurrentThread(&env, nullptr);
        if (status != JNI_OK) {
            LOGCATE("FFMediaPlayer::GetJNIEnv failed to attach current thread");
            return nullptr;
        }
        *isAttach = true;
    }
    return env;
}

jobject FFMediaPlayer::GetJavaObj() {
    return m_JavaObj;
}

JavaVM *FFMediaPlayer::GetJavaVM() {
    return m_JavaVM;
}

void FFMediaPlayer::PostMessage(void *context, int msgType, float msgCode) {
    if (context != nullptr) {
        FFMediaPlayer *player = static_cast<FFMediaPlayer *>(context);
        bool isAttach = false;
        JNIEnv *env = player->GetJNIEnv(&isAttach);
        LOGCATE("FFMediaPlayer::PostMessage env=%p", env);
        if (env == nullptr)
            return;
        jobject javaObj = player->GetJavaObj();
        jmethodID mid = env->GetMethodID(env->GetObjectClass(javaObj),
                                         JAVA_PLAYER_EVENT_CALLBACK_API_NAME, "(IF)V");
        env->CallVoidMethod(javaObj, mid, msgType, msgCode);
        if (isAttach)
            player->GetJavaVM()->DetachCurrentThread();

    }
}
