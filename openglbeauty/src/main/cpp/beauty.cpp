#include <jni.h>
#include <string>
#include <DynimicMesh.h>
#include "LUTBeautyRender.h"
#include "BaseBeautyRender.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_blabla_beauty_BeautyFBORender_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}



extern "C"
JNIEXPORT jlong JNICALL
Java_com_blabla_beauty_BeautyRender_native_1create(JNIEnv *env, jobject thiz,

                                                   jint beauty_type) {

    BaseBeautyRender *beautyRender = nullptr;
    if (beauty_type == 1) {
        beautyRender = new DynimicMesh();
    }
    if (beauty_type == 2) {
        beautyRender = new LUTBeautyRender();
    }
    return reinterpret_cast<jlong>(beautyRender);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_blabla_beauty_BeautyRender_native_1rendRGBAFrame(JNIEnv *env, jobject thiz,
                                                          jint beauty_type,
                                                          jlong native_handler,
                                                          jbyteArray byte_array,
                                                          jint width,
                                                          jint height) {

    BaseBeautyRender *beautyRender = nullptr;

    beautyRender = reinterpret_cast<BaseBeautyRender *>(native_handler);

    jbyte *c_array = env->GetByteArrayElements(byte_array, 0);
    beautyRender->rendRGBAFrame(reinterpret_cast<uint8_t *>(c_array), width, height);
    env->ReleaseByteArrayElements(byte_array, c_array,
                                  0);

}
extern "C"
JNIEXPORT void JNICALL
Java_com_blabla_beauty_BeautyRender_native_1release(JNIEnv *env, jobject thiz, jint beauty_type,
                                                    jlong native_handler) {
    BaseBeautyRender *beautyRender = nullptr;

    beautyRender = reinterpret_cast<BaseBeautyRender *>(native_handler);

    if (beautyRender) {
        delete beautyRender;
    }
    beautyRender = nullptr;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_blabla_beauty_BeautyRender_native_1setLUTTexure(JNIEnv *env, jobject thiz,
                                                         jint beauty_type, jlong native_handler,
                                                         jint lut_texture_id) {
    LUTBeautyRender *beautyRender = nullptr;
    beautyRender = reinterpret_cast<LUTBeautyRender *>(native_handler);
    beautyRender->setLUTTextureId(lut_texture_id);

}