#include <jni.h>
#include <string>
#include <MediaRecorderContext.h>

extern "C" JNIEXPORT jstring JNICALL
Java_com_hapi_rtmpliving_NativeLib_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_hapi_rtmpliving_RTMPPusherContext_native_1CreateContext(JNIEnv *env, jobject thiz) {

    MediaRecorderContext::CreateContext(env, thiz);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_hapi_rtmpliving_RTMPPusherContext_native_1DestroyContext(JNIEnv *env, jobject thiz) {
    MediaRecorderContext::DeleteContext(env, thiz);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_hapi_rtmpliving_RTMPPusherContext_native_1startPush(JNIEnv *env, jobject thiz,
                                                             jstring out_url, jint frame_width,
                                                             jint frame_height, jint video_bit_rate,
                                                             jint fps, jint audio_sample_rate,
                                                             jint audio_channel_count,
                                                             jint audio_sample_format) {

    const char *url = env->GetStringUTFChars(out_url, nullptr);
    MediaRecorderContext *pContext = MediaRecorderContext::GetContext(env, thiz);
    env->ReleaseStringUTFChars(out_url, url);
    if (pContext)
        return pContext->StartRecord(url,
                                     frame_width,
                                     frame_height,
                                     video_bit_rate,
                                     fps, audio_sample_rate, audio_channel_count,audio_sample_format);

}
extern "C"
JNIEXPORT void JNICALL
Java_com_hapi_rtmpliving_RTMPPusherContext_native_1OnAudioData(JNIEnv *env, jobject thiz,
                                                               jbyteArray j_array) {

    jbyte *c_array = env->GetByteArrayElements(j_array, 0);
    int len_arr = env->GetArrayLength(j_array);
    MediaRecorderContext *pContext = MediaRecorderContext::GetContext(env, thiz);
    if (pContext) pContext->OnAudioData(reinterpret_cast<uint8_t *>(c_array), len_arr);
    env->
            ReleaseByteArrayElements(j_array, c_array,
                                     0);

}
extern "C"
JNIEXPORT void JNICALL
Java_com_hapi_rtmpliving_RTMPPusherContext_native_1OnVideoData(JNIEnv *env, jobject thiz,
                                                               jint format, jbyteArray j_array,
                                                               jint width, jint height) {
    MediaRecorderContext *pContext = MediaRecorderContext::GetContext(env, thiz);
    jbyte *c_array = env->GetByteArrayElements(j_array, 0);
    int len_arr = env->GetArrayLength(j_array);
    pContext->OnVideoFrame(format, reinterpret_cast<uint8_t *>(c_array), width, height,
                           len_arr, 0, 0);
    env->ReleaseByteArrayElements(j_array, c_array, 0);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_hapi_rtmpliving_RTMPPusherContext_native_1OnVideoDataRgba(JNIEnv *env, jobject thiz,
                                                                   jbyteArray j_array, jint width,
                                                                   jint height, jint pixel_stride,
                                                                   jint row_padding) {
    MediaRecorderContext *pContext = MediaRecorderContext::GetContext(env, thiz);
    jbyte *c_array = env->GetByteArrayElements(j_array, 0);
    int len_arr = env->GetArrayLength(j_array);
    pContext->OnVideoFrame(IMAGE_FORMAT_RGBA, reinterpret_cast<uint8_t *>(c_array), width,
                           height,
                           len_arr, pixel_stride, row_padding);
    env->ReleaseByteArrayElements(j_array, c_array, 0);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_hapi_rtmpliving_RTMPPusherContext_native_1StopRecord(JNIEnv *env, jobject thiz) {
    MediaRecorderContext *pContext = MediaRecorderContext::GetContext(env, thiz);
    pContext->StopRecord();
}