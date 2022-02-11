
#include "NativeMediaMuxer.h"

void NativePusher::init() {
    mVideoEncoder = new HWVideoEncoder();
    mAudioEncoder = new HWAudioEncoder();
}

void NativePusher::unInit() {
    delete mVideoEncoder;
    delete mAudioEncoder;
    mVideoEncoder = nullptr;
    mAudioEncoder = nullptr;
}

int NativePusher::start(const char *url, RecorderParam *param) {
    strcpy(m_OutUrl, url);
    mp4Muxer = new MP4Muxer(m_OutUrl);

    int result = mVideoEncoder->start(mp4Muxer, param);
    if (result < 0) {
        return result;
    }
    result = mAudioEncoder->start(mp4Muxer, param);
    if (result < 0) {
        return result;
    }

    m_EncoderState = STATE_DECODING;
    if (videoEncoderThread == nullptr)
        videoEncoderThread = new thread(startVideoMediaEncodeThread, this);
    if (audioEncoderThread == nullptr)
        audioEncoderThread = new thread(startAudioMediaEncodeThread, this);
    return result;
}

void NativePusher::startVideoMediaEncodeThread(NativePusher *recorder) {
    recorder->loopVideoEncoder();
}
void NativePusher::startAudioMediaEncodeThread(NativePusher *recorder) {
    recorder->loopAudioEncoder();
}

void NativePusher::loopVideoEncoder() {
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
        mVideoEncoder->dealOneFrame(mp4Muxer);
    }
}
void NativePusher::loopAudioEncoder() {
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
        mAudioEncoder->dealOneFrame(mp4Muxer);
    }
}

void NativePusher::pause() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_EncoderState = STATE_PAUSE;
}

void NativePusher::resume() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_EncoderState = STATE_DECODING;
    m_Cond.notify_all();
}

void NativePusher::stop() {
    m_Exit = true;
    m_EncoderState = STATE_STOP;

    mVideoEncoder->stop();
    mAudioEncoder->stop();
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

    mVideoEncoder->flush(mp4Muxer);
    mAudioEncoder->flush(mp4Muxer);
    LOGCATE("MediaRecorder::StopRecord  encoderThread->join() f");
    mVideoEncoder->clear();
    mAudioEncoder->clear();
    delete mp4Muxer;
}

int NativePusher::onFrame2Encode(AudioFrame *inputFrame) {
    if (m_EncoderState != STATE_DECODING || mAudioEncoder->getQueueSize() > 5) {
        delete inputFrame;
        inputFrame = nullptr;
        return 0;
    }
    mAudioEncoder->pushImg(inputFrame);
    return 1;
}

int NativePusher::onFrame2Encode(VideoFrame *inputFrame) {
    if (m_EncoderState != STATE_DECODING || mVideoEncoder->getQueueSize() > 5) {
        NativeImageUtil::FreeNativeImage(inputFrame);
        delete inputFrame;
        inputFrame = nullptr;
        return 0;
    }
    mVideoEncoder->pushImg(inputFrame);
    return 1;
}
