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
            LOGCATE("MediaRecorder::AddStream Could not find encoder for '%s'",
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
        LOGCATE("MediaRecorder::AddStream Could not alloc an encoding context");
        return -1;
    }

    mCodecCtx->sample_fmt = (mAdioCodec)->sample_fmts ?
                            (mAdioCodec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    mCodecCtx->bit_rate = 96000;
    mCodecCtx->sample_rate = m_RecorderParam.audioSampleRate;
    mCodecCtx->channel_layout = m_RecorderParam.channelLayout;
    mCodecCtx->channels = av_get_channel_layout_nb_channels(mCodecCtx->channel_layout);
    mAvStream->time_base = (AVRational) {1, mCodecCtx->sample_rate};


    /**
     * OPEN AUDIO
     */
    LOGCATE("MediaRecorder::OpenAudio");

    int nb_samples;
    int ret;

    /* open it */
    ret = avcodec_open2(mCodecCtx, mAdioCodec, nullptr);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenAudio Could not open audio codec: %s", av_err2str(ret));
        return -1;
    }

    if (mCodecCtx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = mCodecCtx->frame_size;

    m_pFrame = AllocAudioFrame(mCodecCtx->sample_fmt, mCodecCtx->channel_layout,
                               mCodecCtx->sample_rate, nb_samples);
    m_pTmpFrame = av_frame_alloc();

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(mAvStream->codecpar, mCodecCtx);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenAudio Could not copy the stream parameters");
        return -1;
    }

    /* create resampler context */
    m_pSwrCtx = swr_alloc();
    if (!m_pSwrCtx) {
        LOGCATE("MediaRecorder::OpenAudio Could not allocate resampler context");
        return -1;
    }

    /* set options */
    av_opt_set_int(m_pSwrCtx, "in_channel_count", mCodecCtx->channels, 0);
    av_opt_set_int(m_pSwrCtx, "in_sample_rate", mCodecCtx->sample_rate, 0);
    av_opt_set_sample_fmt(m_pSwrCtx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(m_pSwrCtx, "out_channel_count", mCodecCtx->channels, 0);
    av_opt_set_int(m_pSwrCtx, "out_sample_rate", mCodecCtx->sample_rate, 0);
    av_opt_set_sample_fmt(m_pSwrCtx, "out_sample_fmt", mCodecCtx->sample_fmt, 0);

    if(m_pSwrCtx){
        LOGCATE("MediaRecorder::swr_init(m_pSwrCtx)");
        ret = swr_init(m_pSwrCtx);
        /* initialize the resampling context */
        if ((ret) < 0) {
            LOGCATE("MediaRecorder::OpenAudio Failed to initialize the resampling context");
            return -1;
        }
    }
    return 1;
}

AVFrame *SoftAudioEncoder::AllocAudioFrame(AVSampleFormat sample_fmt, uint64_t channel_layout,
                                           int sample_rate, int nb_samples) {
    LOGCATE("MediaRecorder::AllocAudioFrame");
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        LOGCATE("MediaRecorder::AllocAudioFrame Error allocating an audio frame");
        return nullptr;
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            LOGCATE("MediaRecorder::AllocAudioFrame Error allocating an audio buffer");
            return nullptr;
        }
    }

    return frame;
}


void SoftAudioEncoder::stop() {
    m_Exit = true;
    avcodec_free_context(&mCodecCtx);
    av_frame_free(&m_pFrame);

    while (!m_AudioFrameQueue.Empty()) {
        AudioFrame *pImage = m_AudioFrameQueue.Pop();
        delete pImage;
        pImage = nullptr;
    }

    sws_freeContext(m_pSwsCtx);
    swr_free(&m_pSwrCtx);
    if (m_pTmpFrame != nullptr) {
        av_free(m_pTmpFrame);
        m_pTmpFrame = nullptr;
    }

   // delete mAdioCodec;
    mAdioCodec = nullptr;
}
AVRational SoftAudioEncoder::getTimeBase() {
    return mCodecCtx->time_base;
}

int SoftAudioEncoder::dealOneFrame() {
    LOGCATE("MediaRecorder::EncodeAudioFrame");
    if(m_Exit){
        return -1;
    }
    int result = 0;
    AVCodecContext *c;
    AVPacket pkt = {0}; // data and size must be 0;
    AVFrame *frame;
    int ret;
    int dst_nb_samples;

    av_init_packet(&pkt);


    while (m_AudioFrameQueue.Empty() && !m_Exit) {
        usleep(10 * 1000);
    }

    AudioFrame *audioFrame = m_AudioFrameQueue.Pop();
    frame = m_pTmpFrame;
    if (audioFrame) {
        frame->data[0] = audioFrame->data;
        frame->nb_samples = audioFrame->dataSize / 4;
        frame->pts = mNextPts;
        mNextPts += frame->nb_samples;
    }

    if ((m_AudioFrameQueue.Empty() && m_Exit) || m_EncodeEnd) frame = nullptr;

    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(
                swr_get_delay(m_pSwrCtx, c->sample_rate) + frame->nb_samples,
                c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(m_pFrame);
        if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame Error while av_frame_make_writable");
            result = 1;
            goto EXIT;
        }

        /* convert to destination format */
        ret = swr_convert(m_pSwrCtx,
                          m_pFrame->data, dst_nb_samples,
                          (const uint8_t **) frame->data, frame->nb_samples);
        LOGCATE("MediaRecorder::EncodeAudioFrame dst_nb_samples=%d, frame->nb_samples=%d",
                dst_nb_samples, frame->nb_samples);
        if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame Error while converting");
            result = 1;
            goto EXIT;
        }
        frame = m_pFrame;

        frame->pts = av_rescale_q(m_SamplesCount, (AVRational) {1, c->sample_rate}, c->time_base);
        m_SamplesCount += dst_nb_samples;
    }

    ret = avcodec_send_frame(c, frame);
    if (ret == AVERROR_EOF) {
        result = 1;
        goto EXIT;
    } else if (ret < 0) {
        LOGCATE("MediaRecorder::EncodeAudioFrame audio avcodec_send_frame fail. ret=%s",
                av_err2str(ret));
        result = 0;
        goto EXIT;
    }

    while (!ret) {
        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            result = 0;
            goto EXIT;
        } else if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame audio avcodec_receive_packet fail. ret=%s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
        LOGCATE("MediaRecorder::EncodeAudioFrame pkt pts=%ld, size=%d", pkt.pts, pkt.size);
        int result = WritePacket(m_AVFormatContext, &c->time_base, mAvStream, &pkt);
        if (result < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame audio Error while writing audio frame: %s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
    }
    EXIT:
    if (audioFrame) delete audioFrame;
    return result;
}
