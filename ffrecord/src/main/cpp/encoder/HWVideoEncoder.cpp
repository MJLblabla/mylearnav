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
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, 19);
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
    AMediaCodec_start(pMediaCodec);
    m_Exit = false;
    m_EncodeEnd = 0;
    return 1;

}

void HWVideoEncoder::stop() {
    m_Exit = true;
    // pMediaCodec;
}

void HWVideoEncoder::clear() {
    AMediaCodec_stop(pMediaCodec);
    AMediaCodec_delete(pMediaCodec);
    pMediaCodec = nullptr;
    while (!mVideoFrameQueue.Empty()) {
        VideoFrame *pImage = mVideoFrameQueue.Pop();
        NativeImageUtil::FreeNativeImage(pImage);
        if (pImage) delete pImage;
    }
    if (m_Packet) {
        av_packet_free(&m_Packet);
    }
}


int HWVideoEncoder::dealOneFrame() {
    LOGCATE("HWVideoEncoder::dealOneFrame ");
    if (m_Exit) {
        return -1;
    }
    if (m_Packet == nullptr) {
        m_Packet = av_packet_alloc();
    }

    ssize_t bufidx = AMediaCodec_dequeueInputBuffer(pMediaCodec, 0);
    while (mVideoFrameQueue.Empty() && !m_Exit) {
        usleep(10 * 1000);
    }
    AVPixelFormat srcPixFmt = AV_PIX_FMT_YUV420P;
    VideoFrame *videoFrame = nullptr;
    if (bufidx >= 0) {
        videoFrame = mVideoFrameQueue.Pop();
        size_t bufsize;
        int64_t pts = mNextPts++;
        uint8_t *buf = AMediaCodec_getInputBuffer(pMediaCodec, bufidx, &bufsize);
        //填充yuv数据
        int frameLenYuv = m_RecorderParam.frameWidth * m_RecorderParam.frameHeight * 3 / 2;
        //  LOGCATE("HWVideoEncoder::AMediaCodec_getInputBuffer  bufsize %s %d",bufsize,frameLenYuv);
        memcpy(buf, videoFrame->ppPlane[0], bufsize);

        AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0, frameLenYuv, pts, 0);
    }
    AMediaCodecBufferInfo info;
    //取输出buffer
    auto outindex = AMediaCodec_dequeueOutputBuffer(pMediaCodec, &info, 1000);
    if (outindex >= 0 && m_AVFormatContext != nullptr && !m_Exit) {
        //在这里取走编码后的数据
        //释放buffer给编码器
        size_t outsize;
        uint8_t *buf = AMediaCodec_getOutputBuffer(pMediaCodec, outindex, &outsize);

        //BUFFER_FLAG_KEY_FRAME =1
        bool isKeyFrame = (info.flags & 1) != 0;

        AVPacket *packet = av_packet_alloc();

        av_init_packet(packet);

        packet->stream_index = mAvStream->id;

        packet->data = buf;

        packet->size = outsize;

        packet->pts = av_rescale_q(info.presentationTimeUs, {1, 1000000}, mAvStream->time_base);

        if (isKeyFrame) packet->flags |= AV_PKT_FLAG_KEY;

        LOGCATE("HWVideoEncoder::AMediaCodec_getOutputBuffer  bufsize");
        int result = WritePacket(m_AVFormatContext, &(mAvStream->time_base), mAvStream, m_Packet);
        if (result < 0) {
            LOGCATE("HWVideoEncoder::EncodeVideoFrame video Error while writing audio frame: %s",
                    av_err2str(result));
            result = 0;
        }
        AMediaCodec_releaseOutputBuffer(pMediaCodec, outindex, false);
    }

    av_packet_unref(m_Packet);
    NativeImageUtil::FreeNativeImage(videoFrame);
    if (videoFrame) delete videoFrame;
    return 0;
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