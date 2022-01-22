//
// Created by manjialle on 2022/1/21.
//
#include <unistd.h>
#include "DeMuxer.h"
#include "VideoSoftDecoder.h"
#include "VideoHWDecoder.h"

void DeMuxer::init(DecoderType decoderType) {
    //1.创建封装格式上下文
    m_AVFormatContext = avformat_alloc_context();
    m_AudioDecoder = new AudioDecoder();
    m_AudioDecoder->init();

    if (decoderType == SOFT) {
    }
    m_VideoDecoder = new VideoSoftDecoder();
    m_VideoDecoder->init();
    m_VideoDecoder->SetAVSyncCallback(m_AudioDecoder,
                                      AudioDecoder::GetVideoDecoderTimestampForAVSync);
}


void DeMuxer::unInit() {

    stop();
    if (decoderThread) {
        decoderThread->join();
        delete decoderThread;
        decoderThread = nullptr;
    }

    if (m_AudioDecoder) {
        delete m_AudioDecoder;
        m_AudioDecoder = nullptr;
    }

    if (m_VideoDecoder) {
        delete m_VideoDecoder;
        m_VideoDecoder = nullptr;
    }

    if (m_AVFormatContext != nullptr) {
        avformat_close_input(&m_AVFormatContext);
        avformat_free_context(m_AVFormatContext);
        m_AVFormatContext = nullptr;
    }
}

void DeMuxer::start(const char *url) {

    std::unique_lock<std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_DECODING;
    m_Cond.notify_all();

    int result = -1;
    strcpy(m_Url, url);

    do {
        //2.打开文件
        if (avformat_open_input(&m_AVFormatContext, m_Url, NULL, NULL) != 0) {
            LOGCATE("DecoderBase::InitFFDecoder avformat_open_input fail.");
            break;
        }
        //3.获取音视频流信息
        if (avformat_find_stream_info(m_AVFormatContext, NULL) < 0) {
            LOGCATE("DecoderBase::InitFFDecoder avformat_find_stream_info fail.");
            break;
        }

        int m_audio_streamIndex = -1;
        int m_video_streamIndex = -1;

        //4.获取音视频流索引
        for (int i = 0; i < m_AVFormatContext->nb_streams; i++) {
            if (m_AVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                m_audio_streamIndex = i;

            } else if (m_AVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                m_video_streamIndex = i;
            }
        }

        m_Duration = m_AVFormatContext->duration / AV_TIME_BASE * 1000;//us to ms

        m_AudioDecoder->setStreamIdx(m_audio_streamIndex);
        m_VideoDecoder->setStreamIdx(m_video_streamIndex);

        m_AudioDecoder->start(m_AVFormatContext);
        m_VideoDecoder->start(m_AVFormatContext);

    } while (false);

    if (decoderThread) {
        decoderThread->join();
        delete decoderThread;
        decoderThread = nullptr;
    }
    decoderThread = new thread(doAVDecoding, this);
}


void DeMuxer::stop() {
    LOGCATE("DecoderBase::Stop");
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_STOP;
    m_Cond.notify_all();
    if (decoderThread) {
        decoderThread->join();
        delete decoderThread;
        decoderThread = nullptr;
    }
    m_VideoDecoder->stop();
    m_AudioDecoder->stop();
}

void DeMuxer::pause() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_PAUSE;
    m_VideoDecoder->pause();
    m_AudioDecoder->pause();
}

void DeMuxer::resume() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_DECODING;
    m_Cond.notify_all();
    m_VideoDecoder->resume();
    m_AudioDecoder->resume();
}

void DeMuxer::seekToPosition(float position) {
    LOGCATE("DecoderBase::SeekToPosition position=%f", position);
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_SeekPosition = position;
    m_DecoderState = STATE_DECODING;
    m_Cond.notify_all();
}

long DeMuxer::getCurrentPosition() {
    return m_AudioDecoder->getCurrentPosition();
}

void DeMuxer::setMessageCallback(void *context, MessageCallback callback) {
    m_VideoDecoder->setMessageCallback(context, callback);
    m_AudioDecoder->setMessageCallback(context, callback);
}

void DeMuxer::doAVDecoding(DeMuxer *deMuxer) {
    deMuxer->decodingLoop();
}

void DeMuxer::decodingLoop() {
    LOGCATE("DecoderBase::DecodingLoop start, m_MediaType");
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_DecoderState = STATE_DECODING;
        lock.unlock();
    }
    for (;;) {
        while (m_DecoderState == STATE_PAUSE) {
            std::unique_lock<std::mutex> lock(m_Mutex);
            LOGCATE("DecoderBase::DecodingLoop waiting, m_MediaType");
            m_Cond.wait_for(lock, std::chrono::milliseconds(10));
        }

        if (m_DecoderState == STATE_STOP) {
            break;
        }

        if (decodeOnePacket() != 0) {
            LOGCATE("DecoderBase::Stop");
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_DecoderState = STATE_FINISH;
            m_Cond.notify_all();
            break;
        }
    }
    LOGCATE("DecoderBase::DecodingLoop end");
}

int DeMuxer::decodeOnePacket() {
    if (m_SeekPosition > 0) {
        //seek to frame
        int64_t seek_target = static_cast<int64_t>(m_SeekPosition * 1000000);//微秒
        int64_t seek_min = INT64_MIN;
        int64_t seek_max = INT64_MAX;
        int seek_ret = avformat_seek_file(m_AVFormatContext, -1, seek_min, seek_target, seek_max,
                                          0);
        if (seek_ret < 0) {
            m_SeekSuccess = false;
            LOGCATE("BaseDecoder::DecodeOneFrame error while seeking m_MediaType");
        } else {
            m_VideoDecoder->clearCache();
            m_AudioDecoder->clearCache();
            m_SeekSuccess = true;
            m_SeekPosition = -1;
            LOGCATE("BaseDecoder::DecodeOneFrame seekFrame pos=%f", m_SeekPosition);
        }
    }

    AVPacket avPacket = {0};
    int result = av_read_frame(m_AVFormatContext, &avPacket);
    if (result >= 0) {
        int buffersizeVideo = m_VideoDecoder->getBufferSize();
        int buffersizeAudio = m_AudioDecoder->getBufferSize();
        //防止缓冲数据包过多
        while ((buffersizeVideo > 5 || buffersizeAudio > 5) && m_DecoderState == STATE_DECODING &&
               m_SeekPosition < 0) {
            buffersizeVideo = m_VideoDecoder->getBufferSize();
            buffersizeAudio = m_AudioDecoder->getBufferSize();
            usleep(5 * 1000);
        }
        if (avPacket.stream_index == m_VideoDecoder->getStreamIdx()) {
            m_VideoDecoder->pushAVPacket(&avPacket);
        } else if (avPacket.stream_index == m_AudioDecoder->getStreamIdx()) {
            m_AudioDecoder->pushAVPacket(&avPacket);
        } else {
            av_packet_unref(&avPacket);
        }
        return 0;
    } else {
        return -1;
    }
}
