#include <jni.h>
#include <string>
#include "play/FFMediaPlayer.h"
#include "play/render/video/VideoGLRender.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_mjl_ffplay_NativeLib_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


extern "C"
JNIEXPORT jlong JNICALL
Java_com_mjl_ffplay_FFPlayEngine_nativeCreateFFPlayEngine(JNIEnv *env, jobject thiz) {
    FFMediaPlayer *ffMediaPlayer = new FFMediaPlayer();
    return reinterpret_cast<jlong>(ffMediaPlayer);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_FFPlayEngine_nativeDestroy(JNIEnv *env, jobject thiz, jlong native_engine_ctr) {
    FFMediaPlayer *ffMediaPlayer = reinterpret_cast<FFMediaPlayer *>(native_engine_ctr);
    ffMediaPlayer->UnInit();
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_mjl_ffplay_FFPlayEngine_nativeGetMediaParams(JNIEnv *env, jobject thiz,
                                                      jlong native_engine_ctr, jint param_type) {
    FFMediaPlayer *ffMediaPlayer = reinterpret_cast<FFMediaPlayer *>(native_engine_ctr);
    ffMediaPlayer->GetMediaParams(param_type);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_FFPlayEngine_nativePause(JNIEnv *env, jobject thiz, jlong native_engine_ctr) {
    FFMediaPlayer *ffMediaPlayer = reinterpret_cast<FFMediaPlayer *>(native_engine_ctr);
    ffMediaPlayer->Pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_FFPlayEngine_nativePlay(JNIEnv *env, jobject thiz, jlong native_engine_ctr,jstring jurl) {
    const char *url = env->GetStringUTFChars(jurl, nullptr);
    FFMediaPlayer *ffMediaPlayer = reinterpret_cast<FFMediaPlayer *>(native_engine_ctr);
    ffMediaPlayer->Play(const_cast<char *>(url));
    env->ReleaseStringUTFChars(jurl, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_FFPlayEngine_nativeSeekToPosition(JNIEnv *env, jobject thiz,
                                                      jlong native_engine_ctr, jfloat position) {
    FFMediaPlayer *ffMediaPlayer = reinterpret_cast<FFMediaPlayer *>(native_engine_ctr);
    ffMediaPlayer->SeekToPosition(position);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_FFPlayEngine_nativeSetUpUrl(JNIEnv *env, jobject thiz, jlong native_engine_ctr) {
    FFMediaPlayer *ffMediaPlayer = reinterpret_cast<FFMediaPlayer *>(native_engine_ctr);
    ffMediaPlayer->Init(env, thiz);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_FFPlayEngine_nativeSetVideoPlayerRender(JNIEnv *env, jobject thiz,
                                                            jlong native_engine_ctr, jlong render) {

    FFMediaPlayer *ffMediaPlayer = reinterpret_cast<FFMediaPlayer *>(native_engine_ctr);
    VideoRender *vrender = reinterpret_cast<VideoRender *>(render);
    ffMediaPlayer->setVideoPlayerRender(vrender);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_FFPlayEngine_nativeStop(JNIEnv *env, jobject thiz, jlong native_engine_ctr) {
    FFMediaPlayer *ffMediaPlayer = reinterpret_cast<FFMediaPlayer *>(native_engine_ctr);
    ffMediaPlayer->Stop();
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_mjl_ffplay_OpenGLVideoRender_native_1create(JNIEnv *env, jclass clazz) {
    VideoGLRender *vrender = new VideoGLRender();
    return reinterpret_cast<jlong>(vrender);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_OpenGLVideoRender_native_1OnDrawFrame(JNIEnv *env, jclass clazz,
                                                          jlong render_handler) {
    VideoGLRender *vrender = reinterpret_cast<VideoGLRender *>(render_handler);
    vrender->OnDrawFrame();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_OpenGLVideoRender_native_1OnSurfaceChanged(JNIEnv *env, jclass clazz,
                                                               jlong render_handler, jint width,
                                                               jint height) {
    VideoGLRender *vrender = reinterpret_cast<VideoGLRender *>(render_handler);
    vrender->OnSurfaceChanged(width, height);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_OpenGLVideoRender_native_1OnSurfaceCreated(JNIEnv *env, jclass clazz,
                                                               jlong render_handler) {
    VideoGLRender *vrender = reinterpret_cast<VideoGLRender *>(render_handler);
    vrender->OnSurfaceCreated();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_OpenGLVideoRender_native_1SetTouchLoc(JNIEnv *env, jclass clazz,
                                                          jlong render_handler, jfloat touch_x,
                                                          jfloat touch_y) {
    VideoGLRender *vrender = reinterpret_cast<VideoGLRender *>(render_handler);
    vrender->SetTouchLoc(touch_x, touch_y);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mjl_ffplay_OpenGLVideoRender_native_1SetGesture(JNIEnv *env, jclass clazz,
                                                         jlong render_handler,
                                                         jfloat x_rotate_angle,
                                                         jfloat y_rotate_angle, jfloat scale) {
    VideoGLRender *vrender = reinterpret_cast<VideoGLRender *>(render_handler);
    vrender->SetTouchLoc(x_rotate_angle, y_rotate_angle);
}