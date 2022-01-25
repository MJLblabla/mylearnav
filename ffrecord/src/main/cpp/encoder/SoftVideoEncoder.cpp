//
// Created by manjialle on 2022/1/24.
//

#include "SoftVideoEncoder.h"

int SoftVideoEncoder::start(AVFormatContext *formatCtx, RecorderParam *param) {
    m_RecorderParam = *param;
    m_AVFormatContext = formatCtx;
    /**
     * add stream
     */
    if (formatCtx->oformat->video_codec != AV_CODEC_ID_NONE) {
        mVideoCodec = avcodec_find_encoder(formatCtx->oformat->video_codec);
        if (!mVideoCodec) {
            LOGCATE("MediaRecorder::AddStream Could not find encoder for '%s'",
                    avcodec_get_name(formatCtx->video_codec_id));
            return -1;
        }
    } else {
        return -1;
    }

    mAvStream = avformat_new_stream(formatCtx, NULL);
    mAvStream->id = m_AVFormatContext->nb_streams - 1;

    mCodecCtx = avcodec_alloc_context3(mVideoCodec);
    if (!mCodecCtx) {
        LOGCATE("MediaRecorder::AddStream Could not alloc an encoding context");
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

//    if (m_FormatCtx.o->video_codec != AV_CODEC_ID_NONE) {
//        AddStream(&m_VideoStream, m_FormatCtx, &mVideoCodec, m_OutputFormat->video_codec);
//        m_EnableVideo = 1;
//    }
    /**
     * open video
     */
    int ret = 0;
    ret = avcodec_open2(mCodecCtx, mVideoCodec, nullptr);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenVideo Could not open video codec: %s", av_err2str(ret));
        return -1;
    }
    mFrame = AllocVideoFrame(mCodecCtx->pix_fmt, mCodecCtx->width, mCodecCtx->height);
    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(mAvStream->codecpar, mCodecCtx);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenVideo Could not copy the stream parameters");
        return -1;
    }
    return 1;
}

AVFrame *SoftVideoEncoder::AllocVideoFrame(AVPixelFormat pix_fmt, int width, int height) {
    LOGCATE("MediaRecorder::AllocVideoFrame");
    AVFrame *picture;
    int ret;
    picture = av_frame_alloc();
    if (!picture)
        return nullptr;

    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 1);
    if (ret < 0) {
        LOGCATE("MediaRecorder::AllocVideoFrame Could not allocate frame data.");
        return nullptr;
    }
    m_Exit = false;
    return picture;
}

void SoftVideoEncoder::stop() {
    m_Exit = true;
    avcodec_free_context(&mCodecCtx);
    av_frame_free(&mFrame);

    while (!mVideoFrameQueue.Empty()) {
        VideoFrame *pImage = mVideoFrameQueue.Pop();
        NativeImageUtil::FreeNativeImage(pImage);
        if (pImage) delete pImage;
    }
    //delete mVideoCodec;
    mVideoCodec = nullptr;
}

AVRational SoftVideoEncoder::getTimeBase() {
    return mCodecCtx->time_base;
}

int SoftVideoEncoder::dealOneFrame() {
    LOGCATE("MediaRecorder::EncodeVideoFrame");
    int result = 0;
    int ret;
    AVCodecContext *c;
    AVFrame *frame;

    if (m_Packet == nullptr) {
        m_Packet = av_packet_alloc();
    }
    c = mCodecCtx;

    while (mVideoFrameQueue.Empty() && !m_Exit) {
        usleep(10 * 1000);
    }

    frame = mFrame;
    AVPixelFormat srcPixFmt = AV_PIX_FMT_YUV420P;
    VideoFrame *videoFrame = mVideoFrameQueue.Pop();
    if (videoFrame) {
        frame->data[0] = videoFrame->ppPlane[0];
        frame->data[1] = videoFrame->ppPlane[1];
        frame->data[2] = videoFrame->ppPlane[2];
        frame->linesize[0] = videoFrame->pLineSize[0];
        frame->linesize[1] = videoFrame->pLineSize[1];
        frame->linesize[2] = videoFrame->pLineSize[2];
        frame->width = videoFrame->width;
        frame->height = videoFrame->height;
        frame->format = srcPixFmt;
    }
    if ((mVideoFrameQueue.Empty() && m_Exit) || m_EncodeEnd) frame = nullptr;

    if (frame != nullptr) {
        /* when we pass a frame to the encoder, it may keep a reference to it
        * internally; make sure we do not overwrite it here */
        if (av_frame_make_writable(mFrame) < 0) {
            result = 1;
            goto EXIT;
        }
        frame->pts = mNextPts++;
    }

    ret = avcodec_send_frame(c, frame);
    if (ret == AVERROR_EOF) {
        result = 1;
        goto EXIT;
    } else if (ret < 0) {
        LOGCATE("MediaRecorder::EncodeVideoFrame video avcodec_send_frame fail. ret=%s",
                av_err2str(ret));
        result = 0;
        goto EXIT;
    }

    while (!ret) {
        ret = avcodec_receive_packet(c, m_Packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            result = 0;
            goto EXIT;
        } else if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeVideoFrame video avcodec_receive_packet fail. ret=%s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
        LOGCATE("MediaRecorder::EncodeVideoFrame video pkt pts=%ld, size=%d", m_Packet->pts,
                m_Packet->size);
        int result = WritePacket(m_AVFormatContext, &c->time_base, mAvStream, m_Packet);
        if (result < 0) {
            LOGCATE("MediaRecorder::EncodeVideoFrame video Error while writing audio frame: %s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
    }

    EXIT:
    av_packet_unref(m_Packet);
    NativeImageUtil::FreeNativeImage(videoFrame);
    if (videoFrame) delete videoFrame;
    return result;
}


