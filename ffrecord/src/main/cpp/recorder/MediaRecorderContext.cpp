//
// Created by manjialle on 2021/8/20.
//


#include "LogUtil.h"
#include "ImageDef.h"
#include "MediaRecorderContext.h"
#include "libyuv.h"
#include "yuvutil.h"

extern "C" {
#include <libavutil/channel_layout.h>
}

jfieldID MediaRecorderContext::s_ContextHandle = 0L;

MediaRecorderContext::MediaRecorderContext() {
    mEnMuxer = new NativeMediaMuxer();
   //  mEnMuxer = new EnMuxer();
    mEnMuxer->init();
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
                                  int frameHeight, int videoBitRate,
                                  int fps,
                                  int audioSampleRate,
                                  int audioChannelCount,
                                  int audioSampleFormat) {
    isStart = true;
    LOGCATE("MediaRecorderContext::StartRecord recorderType=%d, outUrl=%s, [w,h]=[%d,%d], videoBitRate=%d, fps=%d",
            recorderType, outUrl, frameWidth, frameHeight, videoBitRate, fps);

    std::unique_lock<std::mutex> lock(m_mutex);
    mParam.frameWidth = frameWidth;
    mParam.frameHeight = frameHeight;
    mParam.videoBitRate = videoBitRate;
    mParam.fps = fps;
    mParam.audioSampleRate = audioSampleRate;

    mParam.channelCount = audioChannelCount;
    mParam.sampleDeep = audioSampleFormat;

    if (audioChannelCount == 1) {
        mParam.audioChannelLayout = AV_CH_LAYOUT_MONO;
    } else {
        mParam.audioChannelLayout = AV_CH_LAYOUT_STEREO;
    }

    if (mParam.sampleDeep == 16) {
        mParam.audioSampleFormat = AV_SAMPLE_FMT_S16;
    } else {
        mParam.audioSampleFormat = AV_SAMPLE_FMT_U8;
    }
    mEnMuxer->start(outUrl, &mParam);
    return 0;
}


int MediaRecorderContext::StopRecord() {
    isStart = false;
    LOGCATE("MediaRecorderContext::StopRecord");
    std::unique_lock<std::mutex> lock(m_mutex);
    mEnMuxer->stop();
    if (src_yuv) {
        free(src_yuv);
        src_yuv = nullptr;
    }

    return 0;
}

void MediaRecorderContext::OnAudioData(uint8_t *pData, int size) {
    if (!isStart)
        return;
    AudioFrame *audioFrame = new AudioFrame(pData, size, true);
    mEnMuxer->onFrame2Encode(audioFrame);
}

void
MediaRecorderContext::OnVideoFrame(int format, uint8_t *pBuffer, int widthsrc,
                                   int heightsrc, int size, int pixelStride, int rowPadding) {

    if (!isStart)
        return;

    auto *nativeImage = new NativeImage();

    int src_y_size = widthsrc * heightsrc;
    int src_u_size = (widthsrc >> 1) * (heightsrc >> 1);
    if (src_yuv == nullptr) {
        src_yuv = static_cast<uint8_t *>(malloc(widthsrc * heightsrc * 3 / 2));
    }
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
            libyuv::ABGRToI420(pBuffer, widthsrc * pixelStride + rowPadding,
                               src_yuv, widthsrc,
                               src_yuv + src_y_size, widthsrc / 2,
                               src_yuv + src_y_size + src_u_size, widthsrc / 2,
                               widthsrc, heightsrc);
            break;
    }

    uint8_t *target_yuv = nullptr;
    target_yuv = static_cast<uint8_t *>(malloc(mParam.frameWidth * mParam.frameHeight * 3 / 2));

    YuvUtil_ScaleI420(src_yuv, widthsrc, heightsrc, target_yuv, mParam.frameWidth,
                      mParam.frameHeight, 3);

    int width = mParam.frameWidth;
    int height = mParam.frameHeight;
    int tar_y_size = width * height;
    int tar_u_size = (width >> 1) * (height >> 1);

    nativeImage->width = width;
    nativeImage->height = height;
    nativeImage->format = IMAGE_FORMAT_I420;

    nativeImage->ppPlane[0] = target_yuv;
    nativeImage->ppPlane[1] = target_yuv + tar_y_size;
    nativeImage->ppPlane[2] = target_yuv + tar_y_size + tar_u_size;

    nativeImage->pLineSize[0] = width;
    nativeImage->pLineSize[1] = width / 2;
    nativeImage->pLineSize[2] = width / 2;
    mEnMuxer->onFrame2Encode(nativeImage);

}





