#include <jni.h>
#include <string>

#include <MediaRecorderContext.h>


extern "C" JNIEXPORT jstring JNICALL
Java_com_cxp_nativelibffmpeg_MediaRecorderContext_stringFromJNI__(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_cxp_nativelibffmpeg_MediaRecorderContext_native_1CreateContext__(JNIEnv *env,
                                                                          jobject thiz) {
    MediaRecorderContext::CreateContext(env, thiz);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_cxp_nativelibffmpeg_MediaRecorderContext_native_1DestroyContext__(JNIEnv *env,
                                                                           jobject thiz) {
    MediaRecorderContext::DeleteContext(env, thiz);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_cxp_nativelibffmpeg_MediaRecorderContext_native_1StartRecord__ILjava_lang_String_2IIJI(
        JNIEnv *env, jobject thiz,
        jint recorder_type, jstring out_url,

        jint frame_width, jint frame_height,
        jlong video_bit_rate, jint fps) {

    const char *url = env->GetStringUTFChars(out_url, nullptr);
    MediaRecorderContext *pContext = MediaRecorderContext::GetContext(env, thiz);
    env->ReleaseStringUTFChars(out_url, url);
    if (pContext)
        return pContext->StartRecord(recorder_type, url, frame_width, frame_height, video_bit_rate,
                                     fps);
    return 0;

}

extern "C"
JNIEXPORT void JNICALL
Java_com_cxp_nativelibffmpeg_MediaRecorderContext_native_1OnVideoData__I_3BII(JNIEnv *env,
                                                                              jobject thiz,
                                                                              jint format,
                                                                              jbyteArray j_array,
                                                                              jint width,
                                                                              jint height) {
    MediaRecorderContext *pContext = MediaRecorderContext::GetContext(env, thiz);
    jbyte *c_array = env->GetByteArrayElements(j_array, 0);
    int len_arr = env->GetArrayLength(j_array);
    pContext->OnVideoFrame(thiz, format, reinterpret_cast<uint8_t *>(c_array), width, height, len_arr);
    env->ReleaseByteArrayElements(j_array, c_array, 0);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_cxp_nativelibffmpeg_MediaRecorderContext_native_1StopRecord__(JNIEnv *env, jobject thiz) {
    MediaRecorderContext *pContext = MediaRecorderContext::GetContext(env, thiz);
    pContext->StopRecord();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_cxp_nativelibffmpeg_MediaRecorderContext_native_1OnAudioData(JNIEnv *env, jobject thiz,
                                                                      jbyteArray j_array) {
    jbyte *c_array = env->GetByteArrayElements(j_array, 0);
    int len_arr = env->GetArrayLength(j_array);
    MediaRecorderContext *pContext = MediaRecorderContext::GetContext(env, thiz);
    if (pContext) pContext->OnAudioData(reinterpret_cast<uint8_t *>(c_array), len_arr);
    env->ReleaseByteArrayElements(j_array, c_array, 0);
}