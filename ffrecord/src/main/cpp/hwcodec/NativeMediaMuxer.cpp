
#include "NativeMediaMuxer.h"

void NativeMediaMuxer::init() {
    mVideoEncoder = new HWVideoEncoder();
    mAudioEncoder = new HWAudioEncoder();
}

void NativeMediaMuxer::unInit() {
    delete mVideoEncoder;
    delete mAudioEncoder;
    mVideoEncoder = nullptr;
    mAudioEncoder = nullptr;
}

int NativeMediaMuxer::start(const char *url, RecorderParam *param) {
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
    if (encoderThread == nullptr)
        encoderThread = new thread(startMediaEncodeThread, this);
    return result;
}

void NativeMediaMuxer::startMediaEncodeThread(NativeMediaMuxer *recorder) {
    recorder->loopEncoder();
}

void NativeMediaMuxer::loopEncoder() {
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

        double videoTimestamp = mVideoEncoder->getTimestamp();
        double audioTimestamp = mAudioEncoder->getTimestamp();

        LOGCATE("MediaRecorder::loopEncoder [videoTimestamp, audioTimestamp]=[%lf, %lf]",
                videoTimestamp, audioTimestamp);

        if (audioTimestamp >= videoTimestamp) {
            mVideoEncoder->dealOneFrame(mp4Muxer);
        } else {
            mAudioEncoder->dealOneFrame(mp4Muxer);
        }
    }
}

void NativeMediaMuxer::pause() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_EncoderState = STATE_PAUSE;
}

void NativeMediaMuxer::resume() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_EncoderState = STATE_DECODING;
    m_Cond.notify_all();
}

void NativeMediaMuxer::stop() {
    m_Exit = true;
    m_EncoderState = STATE_STOP;

    mVideoEncoder->stop();
    mAudioEncoder->stop();
    LOGCATE("MediaRecorder::StopRecord  encoderThread->join() b");
    if (encoderThread != nullptr) {
        encoderThread->join();
        delete encoderThread;
        encoderThread = nullptr;
    }
    mVideoEncoder->flush(mp4Muxer);
    mAudioEncoder->flush(mp4Muxer);
    LOGCATE("MediaRecorder::StopRecord  encoderThread->join() f");
    mVideoEncoder->clear();
    mAudioEncoder->clear();
    delete mp4Muxer;
}

int NativeMediaMuxer::onFrame2Encode(AudioFrame *inputFrame) {
    if (m_EncoderState != STATE_DECODING || mAudioEncoder->getQueueSize() > 5) {
        delete inputFrame;
        inputFrame = nullptr;
        return 0;
    }
    mAudioEncoder->pushImg(inputFrame);
    return 1;
}

int NativeMediaMuxer::onFrame2Encode(VideoFrame *inputFrame) {
    if (m_EncoderState != STATE_DECODING || mVideoEncoder->getQueueSize() > 5) {
        NativeImageUtil::FreeNativeImage(inputFrame);
        delete inputFrame;
        inputFrame = nullptr;
        return 0;
    }
    mVideoEncoder->pushImg(inputFrame);
    return 1;
}
