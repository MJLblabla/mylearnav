//
// Created by manjialle on 2022/1/25.
//
#include "EnMuxer.h"

void EnMuxer::init() {
    //  mVideoEncoder = new HWVideoEncoder();
    mVideoEncoder = new SoftVideoEncoder();
    mAudioEncoder = new SoftAudioEncoder();
}

int EnMuxer::onFrame2Encode(AudioFrame *inputFrame) {
//    if (m_EncoderState != STATE_DECODING || mAudioEncoder->getQueueSize() > 5) {
//        delete inputFrame;
//        inputFrame = nullptr;
//        return 0;
//    }
    mAudioEncoder->pushImg(inputFrame);
    return 1;
}

int EnMuxer::onFrame2Encode(VideoFrame *inputFrame) {
//    if (m_EncoderState != STATE_DECODING || mVideoEncoder->getQueueSize() > 5) {
//        NativeImageUtil::FreeNativeImage(inputFrame);
//        delete inputFrame;
//        inputFrame = nullptr;
//        return 0;
//    }
    mVideoEncoder->pushImg(inputFrame);
    return 1;
}

int EnMuxer::start(const char *url, RecorderParam *param) {
    strcpy(m_OutUrl, url);
    int ret = avformat_alloc_output_context2(&m_AVFormatContext, NULL, NULL, m_OutUrl);
    if (!m_AVFormatContext) {
        LOGCATE("MediaRecorder:avformat_alloc_output_context2   %s",
                av_err2str(ret));
        ret = avformat_alloc_output_context2(&m_AVFormatContext, NULL, "mpeg", m_OutUrl);
        LOGCATE("MediaRecorder:: avformat_alloc_output_context2G %s",
                av_err2str(ret));
    }
    int result = 0;
    if (!m_AVFormatContext) {
        result = -1;
        return result;
    }

    mVideoEncoder->start(m_AVFormatContext, param);
    mAudioEncoder->start(m_AVFormatContext, param);
    // mx
    av_dump_format(m_AVFormatContext, 0, m_OutUrl, 1);
    /* open the output file, if needed */

    if (!(m_AVFormatContext->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&m_AVFormatContext->pb, m_OutUrl, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGCATE("MediaRecorder::StartRecord Could not open '%s': %s", m_OutUrl,
                    av_err2str(ret));
            result = -1;
            return result;
        }
    }

    /* Write the stream header, if any. */
    result = avformat_write_header(m_AVFormatContext, nullptr);
    if (result < 0) {
        LOGCATE("MediaRecorder::StartRecord Error occurred when opening output file: %s",
                av_err2str(result));
        result = -1;
        return result;
    }
    m_EncoderState = STATE_DECODING;

    if (videoEncoderThread == nullptr)
        videoEncoderThread = new thread(startVideoMediaEncodeThread, this);
    if (audioEncoderThread == nullptr)
        audioEncoderThread = new thread(startAudioMediaEncodeThread, this);

    return result;
}

void EnMuxer::startVideoMediaEncodeThread(EnMuxer *recorder) {
    recorder->loopVideoEncoder();
}

void EnMuxer::startAudioMediaEncodeThread(EnMuxer *recorder) {
    recorder->loopAudioEncoder();
}

void EnMuxer::loopAudioEncoder() {
    while (!m_Exit) {
        while (m_EncoderState == STATE_PAUSE) {
            std::unique_lock<std::mutex> lock(m_Mutex);
            LOGCATE("DeMuxer::DecodingLoop waiting, m_MediaType");
            m_Cond.wait_for(lock, std::chrono::milliseconds(10));
        }

        if (m_EncoderState == STATE_STOP) {
            LOGCATE("DeMuxer::DecodingLoop  stop break thread");
            break;
        }
        mAudioEncoder->dealOneFrame();
    }
}

void EnMuxer::loopVideoEncoder() {

    while (!m_Exit) {
        while (m_EncoderState == STATE_PAUSE) {
            std::unique_lock<std::mutex> lock(m_Mutex);
            LOGCATE("DeMuxer::DecodingLoop waiting, m_MediaType");
            m_Cond.wait_for(lock, std::chrono::milliseconds(10));
        }
        if (m_EncoderState == STATE_STOP) {
            LOGCATE("DeMuxer::DecodingLoop  stop break thread");
            break;
        }
        mVideoEncoder->dealOneFrame();
    }
}

void EnMuxer::stop() {
    // std::unique_lock<std::mutex> lock(m_Mutex);
    m_Exit = true;
    m_EncoderState = STATE_STOP;
    // m_Cond.notify_all();

    mVideoEncoder->stop();
    LOGCATE("MediaRecorder::StopRecord   mVideoEncoder->stop() f");
    mAudioEncoder->stop();
    LOGCATE("MediaRecorder::StopRecord   mAudioEncoder->stop() f");

    LOGCATE("MediaRecorder::StopRecord  encoderThread->join() b");
    if (videoEncoderThread != nullptr) {
        videoEncoderThread->join();
        delete videoEncoderThread;
        videoEncoderThread = nullptr;
    }
    if (audioEncoderThread != nullptr) {
        audioEncoderThread->join();
        delete audioEncoderThread;
        audioEncoderThread = nullptr;
    }
    LOGCATE("MediaRecorder::StopRecord  encoderThread->join() f");

    mVideoEncoder->clear();
    mAudioEncoder->clear();

    int ret = av_write_trailer(m_AVFormatContext);
    LOGCATE("MediaRecorder::StopRecord while av_write_trailer %s",
            av_err2str(ret));

    if (!(m_AVFormatContext->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&m_AVFormatContext->pb);
    avformat_free_context(m_AVFormatContext);
}

void EnMuxer::pause() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_EncoderState = STATE_PAUSE;
}

void EnMuxer::resume() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_EncoderState = STATE_DECODING;
    m_Cond.notify_all();
}

void EnMuxer::unInit() {
    delete mVideoEncoder;
    mVideoEncoder = nullptr;
    delete mAudioEncoder;
    mAudioEncoder = nullptr;
}



