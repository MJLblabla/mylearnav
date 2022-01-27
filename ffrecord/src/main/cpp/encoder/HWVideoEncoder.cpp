//
// Created by manjialle on 2022/1/25.
//
#include "HWVideoEncoder.h"


int HWVideoEncoder::start(AVFormatContext *formatCtx, RecorderParam *param) {
    m_RecorderParam = *param;
    m_AVFormatContext = formatCtx;

    mAvStream = avformat_new_stream(formatCtx, NULL);
    mAvStream->id = m_AVFormatContext->nb_streams - 1;
    mAvStream->time_base = (AVRational) {1, m_RecorderParam.fps};


    if (formatCtx->oformat->video_codec != AV_CODEC_ID_NONE) {
        mVideoCodec = avcodec_find_encoder(formatCtx->oformat->video_codec);
        if (!mVideoCodec) {
            LOGCATE("SoftVideoEncoder::AddStream Could not find encoder for '%s'",
                    avcodec_get_name(formatCtx->video_codec_id));
            return -1;
        }
    } else {
        return -1;
    }
    mCodecCtx = avcodec_alloc_context3(mVideoCodec);
    if (!mCodecCtx) {
        LOGCATE("SoftVideoEncoder::AddStream Could not alloc an encoding context");
        return -1;
    }
    mCodecCtx->codec_id = formatCtx->video_codec_id;
    mCodecCtx->bit_rate = m_RecorderParam.videoBitRate;
    /* Resolution must be a multiple of two. */
    mCodecCtx->width = m_RecorderParam.frameWidth;
    mCodecCtx->height = m_RecorderParam.frameHeight;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    mAvStream->time_base = (AVRational) {1, m_RecorderParam.fps};

    mCodecCtx->time_base = mAvStream->time_base;

    mCodecCtx->gop_size = 12; /* emit one intra frame every twelve frames at most */
    mCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    if (mCodecCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        mCodecCtx->max_b_frames = 2;
    }

    if (mCodecCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        mCodecCtx->mb_decision = 2;
    }
    /* Some formats want stream headers to be separate. */
    if (formatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        mCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    int ret = 0;
    ret = avcodec_open2(mCodecCtx, mVideoCodec, nullptr);
    if (ret < 0) {
        LOGCATE("SoftVideoEncoder::OpenVideo Could not open video codec: %s", av_err2str(ret));
        return -1;
    }
    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(mAvStream->codecpar, mCodecCtx);


    pMediaCodec = AMediaCodec_createEncoderByType("video/avc");//h264 // 创建 codec 编码器
    if (pMediaCodec == nullptr) {
        LOGCATE("HWVideoEncoder::AMediaCodec_createEncoderByType  error");
        return -1;
    }

    format = AMediaFormat_new();
    AMediaFormat_setString(format, "mime", "video/avc");
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, param->frameWidth);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, param->frameHeight);
    //在OMX_IVCommon.h https://www.androidos.net.cn/android/9.0.0_r8/xref/frameworks/native/headers/media_plugin/media/openmax/OMX_IVCommon.h
    // AMediaFormat_setInt32(format,AMEDIAFORMAT_KEY_COLOR_FORMAT,OMX_COLOR_FormatYUV420Planar);
    //i420
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, 21);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, param->fps);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, param->videoBitRate);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 5);

//这里配置 format
    media_status_t status = AMediaCodec_configure(pMediaCodec, format, NULL, NULL,
                                                  AMEDIACODEC_CONFIGURE_FLAG_ENCODE);//解码，flags 给0，编码给AMEDIACODEC_CONFIGURE_FLAG_ENCODE
    if (status != 0) {
        LOGCATE("HWVideoEncoder::AMediaCodec_configure  error");
        return -1;
    }

    if ((status = AMediaCodec_start(pMediaCodec)) != AMEDIA_OK) {
        LOGCATE("AMediaCodec_start: Could not start encoder.");
        return -1;
    } else {
        LOGCATE("AMediaCodec_start: encoder successfully started");
    }
    m_Exit = false;
    m_EncodeEnd = 0;
    return 1;

}

void HWVideoEncoder::stop() {
    m_Exit = true;
    // pMediaCodec;
}

void HWVideoEncoder::clear() {

    avcodec_free_context(&mCodecCtx);
    //delete mVideoCodec;
    mVideoCodec = nullptr;

    AMediaCodec_stop(pMediaCodec);
    AMediaCodec_delete(pMediaCodec);
    pMediaCodec = nullptr;
    while (!mVideoFrameQueue.Empty()) {
        VideoFrame *pImage = mVideoFrameQueue.Pop();
        NativeImageUtil::FreeNativeImage(pImage);
        if (pImage) delete pImage;
    }

    mVideoCodec = nullptr;
    m_Packet = nullptr;
}


int HWVideoEncoder::dealOneFrame() {
    LOGCATE("HWVideoEncoder::dealOneFrame ");

    if (m_Exit) {
        return -1;
    }

    int result = 0;
    if (m_Packet == nullptr) {
        m_Packet = av_packet_alloc();
    }
    while (mVideoFrameQueue.Empty() && !m_Exit) {
        usleep(10 * 1000);
    }
    VideoFrame *videoFrame = nullptr;
    videoFrame = mVideoFrameQueue.Pop();
    if (m_Exit || videoFrame == nullptr) {
        return -1;
    }
    size_t bufsize;
    ssize_t bufidx = AMediaCodec_dequeueInputBuffer(pMediaCodec, 1);

    if (bufidx >= 0) {
        int pts = mNextPts++;
        uint8_t *buf = AMediaCodec_getInputBuffer(pMediaCodec, bufidx, &bufsize);
        //填充yuv数据

        int frameLenYuv = m_RecorderParam.frameWidth * m_RecorderParam.frameHeight * 3 / 2;
        //  LOGCATE("HWVideoEncoder::AMediaCodec_getInputBuffer  bufsize %s %d",bufsize,frameLenYuv);
        if (m_Exit || buf == nullptr || videoFrame->ppPlane[0] == nullptr) {
            LOGCATE("MediaCodecH264Enc: obtained InputBuffer, but no address.");
        } else {
            LOGCATE("HWVideoEncoder::AMediaCodec_queueInputBuffer  bufsize %zu  bufidx %zd",
                    bufsize, bufidx);
            memcpy(buf, videoFrame->ppPlane[0], frameLenYuv);
            AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0, frameLenYuv,
                                         pts * av_q2d(getTimeBase()), 0);
        }
    }

    AMediaCodecBufferInfo info;
    ssize_t obufidx;
    //int pos = 0;
    /*Second, dequeue possibly pending encoded frames*/
    while ((obufidx = AMediaCodec_dequeueOutputBuffer(pMediaCodec, &info, 0)) >= 0) {
        if (m_AVFormatContext == nullptr || m_Exit) {
            goto EXIT;
        }
        auto oBuf = AMediaCodec_getOutputBuffer(pMediaCodec, obufidx, &bufsize);

        if (oBuf) {
            int m_infoSize = info.size;
            m_Packet->data = static_cast<uint8_t *>(malloc(m_infoSize));
            memcpy(m_Packet->data, oBuf, m_infoSize);
            m_Packet->stream_index = mAvStream->id;
            int pts = av_rescale_q(info.presentationTimeUs, AV_TIME_BASE_Q,
                                   mCodecCtx->time_base);
            m_Packet->size = info.size - info.offset;
            m_Packet->pts = pts;
            m_Packet->dts = pts;
            m_Packet->flags = info.flags;
            result = WritePacket(m_AVFormatContext, &mCodecCtx->time_base, mAvStream, m_Packet);
            if (result < 0) {
                LOGCATE("HWVideoEncoder::EncodeVideoFrame video Error while writing audio frame: %s",
                        av_err2str(result));
                result = 0;
                goto EXIT;
            }
        }

        AMediaCodec_releaseOutputBuffer(pMediaCodec, obufidx, false);
    }

    if (obufidx == AMEDIA_ERROR_UNKNOWN) {
        goto EXIT;
    }

    EXIT:
    av_packet_unref(m_Packet);
    NativeImageUtil::FreeNativeImage(videoFrame);
    delete videoFrame;
    return result;
}

AVRational HWVideoEncoder::getTimeBase() {
    return mAvStream->time_base;
}

int HWVideoEncoder::WritePacket(AVFormatContext *fmt_ctx, AVRational *time_base, AVStream *st,
                                AVPacket *pkt) {
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    return av_interleaved_write_frame(fmt_ctx, pkt);
}