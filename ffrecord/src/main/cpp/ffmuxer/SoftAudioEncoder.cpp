//
// Created by manjialle on 2022/1/24.
//

#include "SoftAudioEncoder.h"

int SoftAudioEncoder::start(AVFormatContext *formatCtx, RecorderParam *param) {
    m_RecorderParam = *param;
    m_AVFormatContext = formatCtx;

    /**
     * add stream
     */
    if (formatCtx->oformat->audio_codec != AV_CODEC_ID_NONE) {
        mAdioCodec = avcodec_find_encoder(formatCtx->oformat->audio_codec);
        if (!mAdioCodec) {
            LOGCATE("SoftAudioEncoder::AddStream Could not find encoder for '%s'",
                    avcodec_get_name(formatCtx->video_codec_id));
            return -1;
        }
    } else {
        return -1;
    }

    mAvStream = avformat_new_stream(formatCtx, NULL);
    mAvStream->id = m_AVFormatContext->nb_streams - 1;

    mCodecCtx = avcodec_alloc_context3(mAdioCodec);
    if (!mCodecCtx) {
        LOGCATE("SoftAudioEncoder::AddStream Could not alloc an encoding context");
        return -1;
    }

    mCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;//float, planar, 4 字节
    mCodecCtx->sample_rate = param->audioSampleRate;
    mCodecCtx->channel_layout = param->audioChannelLayout;
    mCodecCtx->channels = av_get_channel_layout_nb_channels(mCodecCtx->channel_layout);
    mCodecCtx->bit_rate = 22050;

    /* Some formats want stream headers to be separate. */
//    if (formatCtx->oformat->flags & AVFMT_GLOBALHEADER)
//        mCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    /**
     * OPEN AUDIO
     */
    LOGCATE("SoftAudioEncoder::OpenAudio");
    int nb_samples;
    int ret;
    /* open it */
    ret = avcodec_open2(mCodecCtx, mAdioCodec, nullptr);


    if (ret < 0) {
        LOGCATE("SoftAudioEncoder::OpenAudio Could not open audio codec: %s", av_err2str(ret));
        return -1;
    }

    m_pFrame = av_frame_alloc();
    // 每个通道的数量
    m_pFrame->nb_samples = mCodecCtx->frame_size;
    m_pFrame->format = mCodecCtx->sample_fmt;

    m_frameBufferSize = av_samples_get_buffer_size(nullptr, mCodecCtx->channels,
                                                   mCodecCtx->frame_size,
                                                   mCodecCtx->sample_fmt, 1);

    LOGCATE("SingleAudioRecorder::StartRecord m_frameBufferSize=%d, nb_samples=%d",
            m_frameBufferSize, m_pFrame->nb_samples);
    m_pFrameBuffer = (uint8_t *) av_malloc(m_frameBufferSize);
    avcodec_fill_audio_frame(m_pFrame, mCodecCtx->channels, mCodecCtx->sample_fmt,
                             (const uint8_t *) m_pFrameBuffer, m_frameBufferSize, 1);
    av_new_packet(&m_avPacket, m_frameBufferSize);


    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(mAvStream->codecpar, mCodecCtx);
    if (ret < 0) {
        LOGCATE("SoftAudioEncoder::OpenAudio Could not copy the stream parameters");
        return -1;
    }

    /* create resampler context */
    m_pSwrCtx = swr_alloc();
    if (!m_pSwrCtx) {
        LOGCATE("SoftAudioEncoder::OpenAudio Could not allocate resampler context");
        return -1;
    }

    /* set options */
//    av_opt_set_channel_layout(m_pSwrCtx, "in_channel_layout", m_RecorderParam.audioChannelLayout,
//                              0);
//    av_opt_set_channel_layout(m_pSwrCtx, "out_channel_layout", mCodecCtx->channel_layout, 0);
//    av_opt_set_int(m_pSwrCtx, "in_sample_rate", m_RecorderParam.audioSampleRate, 0);
//    av_opt_set_int(m_pSwrCtx, "out_sample_rate", mCodecCtx->sample_rate, 0);
//    av_opt_set_sample_fmt(m_pSwrCtx, "in_sample_fmt",
//                          AVSampleFormat(m_RecorderParam.audioSampleFormat), 0);
//    av_opt_set_sample_fmt(m_pSwrCtx, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);

    m_pSwrCtx = swr_alloc_set_opts(m_pSwrCtx,
                                   m_RecorderParam.audioChannelLayout, AV_SAMPLE_FMT_FLTP,
                                   m_RecorderParam.audioSampleRate,//输出格式
                                   m_RecorderParam.audioChannelLayout, m_RecorderParam.audioSampleFormat,  m_RecorderParam.audioSampleRate, 0,
                                   0);//输入格式

    if (m_pSwrCtx) {
        LOGCATE("SoftAudioEncoder::swr_init(m_pSwrCtx)");
        ret = swr_init(m_pSwrCtx);
        /* initialize the resampling context */
        if ((ret) < 0) {
            LOGCATE("SoftAudioEncoder::OpenAudio Failed to initialize the resampling context");
            return -1;
        }
    }
    LOGCATE("SoftAudioEncoder::start finish");
    m_Exit = false;
    m_EncodeEnd = 0;
    return 1;
}


void SoftAudioEncoder::stop() {
    m_Exit = true;
}

void SoftAudioEncoder::clear() {
    avcodec_free_context(&mCodecCtx);
    av_frame_free(&m_pFrame);

    while (!m_AudioFrameQueue.Empty()) {
        AudioFrame *pImage = m_AudioFrameQueue.Pop();
        delete pImage;
        pImage = nullptr;
    }

    if (m_pSwrCtx) {
        swr_free(&m_pSwrCtx);
    }

    if (m_pFrameBuffer != nullptr) {
        av_free(m_pFrameBuffer);
        m_pFrameBuffer = nullptr;
    }
    // delete mAdioCodec;
    mAdioCodec = nullptr;
}


AVRational SoftAudioEncoder::getTimeBase() {
    return mCodecCtx->time_base;
}

int SoftAudioEncoder::dealOneFrame() {
    LOGCATE("SoftAudioEncoder::EncodeAudioFrame");

    int result = -1;
    while (m_AudioFrameQueue.Empty() && !m_Exit) {
        usleep(10 * 1000);
    }

    AudioFrame *audioFrame = m_AudioFrameQueue.Pop();
    if (!audioFrame) {
        result = -1;
        return result;
    }

    AVFrame *pFrame = m_pFrame;
    result = swr_convert(m_pSwrCtx, pFrame->data, pFrame->nb_samples,
                         (const uint8_t **) &(audioFrame->data),
                         audioFrame->dataSize / m_RecorderParam.channelCount /
                         (m_RecorderParam.sampleDeep / 8));
    if (result < 0) {
        result = -1;
        goto EXIT;
    }

    pFrame->pts = mNextPts;
    mNextPts += m_pFrame->nb_samples;

    result = avcodec_send_frame(mCodecCtx, pFrame);
    if (result < 0) {
        LOGCATE("SingleAudioRecorder::EncodeFrame avcodec_send_frame fail. ret=%d", result);
        return result;
    }
    while (!result) {
        result = avcodec_receive_packet(mCodecCtx, &m_avPacket);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            return 0;
        } else if (result < 0) {
            LOGCATE("SingleAudioRecorder::EncodeFrame avcodec_receive_packet fail. ret=%d", result);
            return result;
        }
        LOGCATE("SingleAudioRecorder::EncodeFrame frame pts=%ld, size=%d", m_avPacket.pts,
                m_avPacket.size);
        m_avPacket.stream_index = mAvStream->index;

        WritePacket(m_AVFormatContext, &mCodecCtx->time_base, mAvStream, &m_avPacket);
        av_packet_unref(&m_avPacket);
    }
    EXIT:
    if (audioFrame) delete audioFrame;
    return result;
}

