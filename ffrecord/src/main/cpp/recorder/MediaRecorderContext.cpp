//
// Created by manjialle on 2021/8/20.
//


#include "LogUtil.h"
#include "ImageDef.h"
#include "MediaRecorderContext.h"
#include "libyuv.h"

jfieldID MediaRecorderContext::s_ContextHandle = 0L;

MediaRecorderContext::MediaRecorderContext() {
    mEnMuxer = new EnMuxer();
    mEnMuxer->init(SOFT);
}

MediaRecorderContext::~MediaRecorderContext() {
    LOGCATE("MediaRecorderContext::~MediaRecorderContext()");
    mEnMuxer->unInit();
    delete mEnMuxer;
}

void MediaRecorderContext::CreateContext(JNIEnv *env, jobject instance) {
    LOGCATE("MediaRecorderContext::CreateContext");
    MediaRecorderContext *pContext = new MediaRecorderContext();
    StoreContext(env, instance, pContext);
}

void
MediaRecorderContext::StoreContext(JNIEnv *env, jobject instance, MediaRecorderContext *pContext) {
    LOGCATE("MediaRecorderContext::StoreContext");
    jclass cls = env->GetObjectClass(instance);
    if (cls == NULL) {
        LOGCATE("MediaRecorderContext::StoreContext cls == NULL");
        return;
    }

    s_ContextHandle = env->GetFieldID(cls, "mNativeContextHandle", "J");
    if (s_ContextHandle == NULL) {
        LOGCATE("MediaRecorderContext::StoreContext s_ContextHandle == NULL");
        return;
    }
    env->SetLongField(instance, s_ContextHandle, reinterpret_cast<jlong>(pContext));
}


void MediaRecorderContext::DeleteContext(JNIEnv *env, jobject instance) {
    LOGCATE("MediaRecorderContext::DeleteContext");
    if (s_ContextHandle == NULL) {
        LOGCATE("MediaRecorderContext::DeleteContext Could not find play.render context.");
        return;
    }

    MediaRecorderContext *pContext = reinterpret_cast<MediaRecorderContext *>(env->GetLongField(
            instance, s_ContextHandle));
    if (pContext) {
        delete pContext;
    }
    env->SetLongField(instance, s_ContextHandle, 0L);
}

MediaRecorderContext *MediaRecorderContext::GetContext(JNIEnv *env, jobject instance) {
    LOGCATE("MediaRecorderContext::GetContext");

    if (s_ContextHandle == NULL) {
        LOGCATE("MediaRecorderContext::GetContext Could not find play.render context.");
        return NULL;
    }
    MediaRecorderContext *pContext = reinterpret_cast<MediaRecorderContext *>(env->GetLongField(
            instance, s_ContextHandle));
    return pContext;
}


int
MediaRecorderContext::StartRecord(int recorderType, const char *outUrl, int frameWidth,
                                  int frameHeight, long videoBitRate,
                                  int fps) {
    isStart = true;
    LOGCATE("MediaRecorderContext::StartRecord recorderType=%d, outUrl=%s, [w,h]=[%d,%d], videoBitRate=%ld, fps=%d",
            recorderType, outUrl, frameWidth, frameHeight, videoBitRate, fps);
    std::unique_lock<std::mutex> lock(m_mutex);
//    switch (recorderType) {
//        case RECORDER_TYPE_SINGLE_VIDEO:
//            break;
//        case RECORDER_TYPE_SINGLE_AUDIO:
//            break;
//        case RECORDER_TYPE_AV:
//
//
//            m_pAVRecorder = new MediaRecorder(outUrl, &param);
//            m_pAVRecorder->StartRecord();
//            break;
//        default:
//            break;

    //   }
    RecorderParam param = {0};
    param.frameWidth = frameWidth;
    param.frameHeight = frameHeight;
    param.videoBitRate = videoBitRate;
    param.fps = fps;
    param.audioSampleRate = DEFAULT_SAMPLE_RATE;
    param.channelLayout = AV_CH_LAYOUT_STEREO;
    param.sampleFormat = AV_SAMPLE_FMT_S16;
    mEnMuxer->start(outUrl, &param);

    return 0;
}

int MediaRecorderContext::StopRecord() {
    isStart = false;
    LOGCATE("MediaRecorderContext::StopRecord");
    std::unique_lock<std::mutex> lock(m_mutex);
    mEnMuxer->stop();
    return 0;
}

void MediaRecorderContext::OnAudioData(uint8_t *pData, int size) {
    LOGCATE("MediaRecorderContext::OnAudioData pData=%p, dataSize=%d", pData, size);

    if (!isStart)
        return;


    AudioFrame *audioFrame = new AudioFrame(pData, size, true);
    mEnMuxer->onFrame2Encode(audioFrame);
}

void
MediaRecorderContext::OnVideoFrame(void *ctx, int format, uint8_t *pBuffer, int widthsrc,
                                   int heightsrc, int size) {
    LOGCATE("MediaRecorderContext::UpdateFrame format=%d, width=%d, height=%d, pData=%p",
            format, widthsrc, heightsrc, pBuffer);

    if (!isStart)
        return;

    NativeImage *nativeImage = new NativeImage();

    uint8_t *src_yuv = nullptr;
    int src_y_size = widthsrc * heightsrc;
    int src_u_size = (widthsrc >> 1) * (heightsrc >> 1);
    src_yuv = static_cast<uint8_t *>(malloc(widthsrc * heightsrc * 3 / 2));
    switch (format) {
        case IMAGE_FORMAT_I420:
            memcpy(src_yuv, pBuffer, size);
            break;
        case IMAGE_FORMAT_NV12:
            libyuv::NV12ToI420(pBuffer, widthsrc,
                               pBuffer + src_y_size, widthsrc,
                               src_yuv, widthsrc,
                               src_yuv + src_y_size, widthsrc >> 1,
                               src_yuv + src_y_size + src_u_size, widthsrc >> 1,
                               widthsrc, heightsrc);
            break;
        case IMAGE_FORMAT_NV21:

            libyuv::NV21ToI420(pBuffer, widthsrc,
                               pBuffer + src_y_size, widthsrc,
                               src_yuv, widthsrc,
                               src_yuv + src_y_size, widthsrc >> 1,
                               src_yuv + src_y_size + src_u_size, widthsrc >> 1,
                               widthsrc, heightsrc);

            break;
        case IMAGE_FORMAT_RGBA:
            libyuv::ABGRToI420(pBuffer, widthsrc * 4,
                               src_yuv, widthsrc,
                               src_yuv + src_y_size, widthsrc >> 1,
                               src_yuv + src_y_size + src_u_size, widthsrc >> 1,
                               widthsrc, heightsrc);
            //  Jni_RgbaToI420(pBuffer,src_yuv, widthsrc, heightsrc);
            break;
    }

    int width = widthsrc;
    int height = heightsrc;

    nativeImage->width = width;
    nativeImage->height = height;
    nativeImage->format = IMAGE_FORMAT_I420;

    nativeImage->ppPlane[0] = src_yuv;
    nativeImage->ppPlane[1] = src_yuv + src_y_size;
    nativeImage->ppPlane[2] = src_yuv + src_y_size + src_u_size;

    nativeImage->pLineSize[0] = width;
    nativeImage->pLineSize[1] = width / 2;
    nativeImage->pLineSize[2] = width / 2;

    LOGCATE("MediaRecorderContext::UpdateFrame2 format=%d, width=%d, height=%d, pData=%p",
            format, widthsrc, heightsrc, pBuffer);

    mEnMuxer->onFrame2Encode(nativeImage);
    // free(src_yuv);
    src_yuv = nullptr;

}





