//
// Created by manjialle on 2022/1/25.
//
#include "EnMuxer.h"

void EnMuxer::init(EecoderType decoderType) {
    mVideoEncoder = new SoftVideoEncoder();
    mAudioEncoder = new SoftAudioEncoder();
}

int EnMuxer::onFrame2Encode(AudioFrame *inputFrame) {
    if (m_EncoderState != STATE_DECODING) {
        delete inputFrame;
        inputFrame = nullptr;
        return 0;
    }
    mAudioEncoder->pushImg(inputFrame);
    return 1;
}

int EnMuxer::onFrame2Encode(VideoFrame *inputFrame) {
    if (m_EncoderState != STATE_DECODING) {
        NativeImageUtil::FreeNativeImage(inputFrame);
        inputFrame = nullptr;
        return 0;
    }
    mVideoEncoder->pushImg(inputFrame);
    return 1;
}

void EnMuxer::start(const char *url, RecorderParam *param) {
    strcpy(m_OutUrl, url);
    int ret = avformat_alloc_output_context2(&m_AVFormatContext, NULL, NULL, m_OutUrl);
    if (!m_AVFormatContext) {
        LOGCATE("MediaRecorder:avformat_alloc_output_context2   %s",
                av_err2str(ret));
        ret = avformat_alloc_output_context2(&m_AVFormatContext, NULL, "mpeg", m_OutUrl);
        LOGCATE("MediaRecorder:: avformat_alloc_output_context2G %s",
                av_err2str(ret));
    }
    if(!m_AVFormatContext){
        return;
    }

    mVideoEncoder->start(m_AVFormatContext, param);
    mAudioEncoder->start(m_AVFormatContext, param);

    // mx
    av_dump_format(m_AVFormatContext, 0, m_OutUrl, 1);
    /* open the output file, if needed */
    int result = 0;
    if (!(m_AVFormatContext->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&m_AVFormatContext->pb, m_OutUrl, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGCATE("MediaRecorder::StartRecord Could not open '%s': %s", m_OutUrl,
                    av_err2str(ret));
            result = -1;
        }
    }

    /* Write the stream header, if any. */
    result = avformat_write_header(m_AVFormatContext, nullptr);
    if (result < 0) {
        LOGCATE("MediaRecorder::StartRecord Error occurred when opening output file: %s",
                av_err2str(result));
        result = -1;
    }
    m_EncoderState = STATE_DECODING;
    if (encoderThread == nullptr)
        encoderThread = new thread(startMediaEncodeThread, this);
}

void EnMuxer::startMediaEncodeThread(EnMuxer *recorder) {
    recorder->loopEncoder();
}

void EnMuxer::loopEncoder() {
    while (!mAudioEncoder->m_EncodeEnd || !mVideoEncoder->m_EncodeEnd) {

        while (m_EncoderState == STATE_PAUSE) {
            std::unique_lock<std::mutex> lock(m_Mutex);
            LOGCATE("DeMuxer::DecodingLoop waiting, m_MediaType");
            m_Cond.wait_for(lock, std::chrono::milliseconds(10));
        }

        if (m_EncoderState == STATE_STOP) {
            LOGCATE("DeMuxer::DecodingLoop  stop break thread");
            break;
        }

        double videoTimestamp = mVideoEncoder->mNextPts * av_q2d(mVideoEncoder->getTimeBase());
        double audioTimestamp = mAudioEncoder->mNextPts * av_q2d(mAudioEncoder->getTimeBase());

        LOGCATE("MediaRecorder::StartVideoEncodeThread [videoTimestamp, audioTimestamp]=[%lf, %lf]",
                videoTimestamp, audioTimestamp);
        if (!mVideoEncoder->m_EncodeEnd &&
            (mAudioEncoder->m_EncodeEnd ||
             av_compare_ts(mVideoEncoder->mNextPts, mVideoEncoder->getTimeBase(),
                           mAudioEncoder->mNextPts, mAudioEncoder->getTimeBase()) <= 0)) {
            //视频和音频时间戳对齐，人对于声音比较敏感，防止出现视频声音播放结束画面还没结束的情况
            if (audioTimestamp <= videoTimestamp && mAudioEncoder->m_EncodeEnd)
                mVideoEncoder->m_EncodeEnd = 1;
            mVideoEncoder->m_EncodeEnd = mVideoEncoder->dealOneFrame();
        } else {
            mAudioEncoder->m_EncodeEnd = mAudioEncoder->dealOneFrame();
        }
    }
}

void EnMuxer::stop() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Exit = true;
    m_EncoderState = STATE_STOP;
    m_Cond.notify_all();
    LOGCATE("MediaRecorder::StopRecord  encoderThread->join() b");
    if (encoderThread != nullptr) {
        encoderThread->join();
        delete encoderThread;
        encoderThread = nullptr;
    }
    LOGCATE("MediaRecorder::StopRecord  encoderThread->join() f");
    mVideoEncoder->stop();
    LOGCATE("MediaRecorder::StopRecord   mVideoEncoder->stop() f");
    mAudioEncoder->stop();
    LOGCATE("MediaRecorder::StopRecord   mAudioEncoder->stop() f");

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


