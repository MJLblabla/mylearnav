
#include "NativePusher.h"

void NativePusher::init() {
    mVideoEncoder = new HWVideoEncoder();
    mAudioEncoder = new HWAudioEncoder();
    mRTMPPush = new RTMPPush();
}

void NativePusher::unInit() {
    delete mVideoEncoder;
    delete mAudioEncoder;
    mVideoEncoder = nullptr;
    mAudioEncoder = nullptr;
    delete mRTMPPush;
    mRTMPPush = nullptr;
}

void NativePusher::onConnect(int code) {
    if(!code){
        return;
    }
    int result = mVideoEncoder->start(mRTMPPush, param);
    if (result < 0) {
        return;
    }
    result = mAudioEncoder->start(mRTMPPush, param);
    if (result < 0) {
        return;
    }

    m_EncoderState = STATE_DECODING;
    if (videoEncoderThread == nullptr)
        videoEncoderThread = new thread(startVideoMediaEncodeThread, this);
    if (audioEncoderThread == nullptr)
        audioEncoderThread = new thread(startAudioMediaEncodeThread, this);

}

int NativePusher::start(const char *url, RecorderParam *recorderParam) {
    strcpy(m_OutUrl, url);
    this->param = recorderParam;
    ConnectCallBack fun = std::bind(&NativePusher::onConnect, this, std::placeholders::_1);
    mRTMPPush->start(url, fun);
    return 0;
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
        mVideoEncoder->dealOneFrame(mRTMPPush);
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
        mAudioEncoder->dealOneFrame(mRTMPPush);
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

    mVideoEncoder->flush(mRTMPPush);
    mAudioEncoder->flush(mRTMPPush);
    LOGCATE("MediaRecorder::StopRecord  encoderThread->join() f");
    mVideoEncoder->clear();
    mAudioEncoder->clear();

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
