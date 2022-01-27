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

    AVCodecParameters *codecpar = mAvStream->codecpar;
    codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    codecpar->codec_id = AV_CODEC_ID_H264;
    codecpar->width = param->frameWidth;
    codecpar->height = param->frameHeight;


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
    AMediaCodec_stop(pMediaCodec);
    AMediaCodec_delete(pMediaCodec);
    pMediaCodec = nullptr;
    while (!mVideoFrameQueue.Empty()) {
        VideoFrame *pImage = mVideoFrameQueue.Pop();
        NativeImageUtil::FreeNativeImage(pImage);
        if (pImage) delete pImage;
    }
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

        uint8_t *buf = AMediaCodec_getInputBuffer(pMediaCodec, bufidx, &bufsize);
        //填充yuv数据

        int frameLenYuv = m_RecorderParam.frameWidth * m_RecorderParam.frameHeight * 3 / 2;
        //  LOGCATE("HWVideoEncoder::AMediaCodec_getInputBuffer  bufsize %s %d",bufsize,frameLenYuv);
        if (m_Exit || buf == nullptr || videoFrame->ppPlane[0] == nullptr) {
            LOGCATE("MediaCodecH264Enc: obtained InputBuffer, but no address.");
        } else {
            memcpy(buf, videoFrame->ppPlane[0], frameLenYuv);
            int fpts = mNextPts++;
            int64_t pts = av_rescale_q(fpts, mAvStream->time_base, AV_TIME_BASE_Q);
            LOGCATE("HWVideoEncoder::xxAMediaCodec_queueInputBuffer  bufsize %zu  bufidx %zd  pts %ld",
                    bufsize, bufidx, pts);
            AMediaCodec_queueInputBuffer(pMediaCodec, bufidx, 0, frameLenYuv,
                                         pts, 0);
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
            //BUFFER_FLAG_KEY_FRAME
            bool isKeyFrame = (info.flags & 1) != 0;
            memcpy(m_Packet->data, oBuf, m_infoSize);
            m_Packet->stream_index = mAvStream->id;

            int64_t pts =info.presentationTimeUs; //av_rescale_q(info.presentationTimeUs, {1, 1000000}, mAvStream->time_base);

            LOGCATE("HWVideoEncoder::xxAMediaCodec_dequeueOutputBuffer  framesize: %d   id %zd  pts %d",
                    m_infoSize, obufidx, pts);
            m_Packet->size = m_infoSize;

            m_Packet->pts = pts;
            if (isKeyFrame) m_Packet->flags |= AV_PKT_FLAG_KEY;

            if (mAvStream->codecpar->extradata == nullptr) {

                // 复制给视频流extradata
                AVCodecParameters *codecpar = mAvStream->codecpar;
                codecpar->extradata = (uint8_t *) av_mallocz(
                        m_infoSize + AV_INPUT_BUFFER_PADDING_SIZE);
                memcpy(codecpar->extradata, oBuf, m_infoSize);
                codecpar->extradata_size = m_infoSize;
                //写入视频文件头信息，放在文件开头位置
                AVDictionary *dict = nullptr;
                av_dict_set(&dict, "movflags", "faststart", 0);
                int intret = avformat_write_header(m_AVFormatContext, &dict);
                LOGCATE("HWVideoEncoder::EncodeVideoFrame :  写入视频文件头信息，放在文件");
            } else{
                int ret = av_interleaved_write_frame(m_AVFormatContext, m_Packet);
                if (ret < 0) {
                    LOGCATE("HWVideoEncoder::EncodeVideoFrame video Error while writing audio frame: %s",
                            av_err2str(ret));
                    result = 0;
                    goto EXIT;
                }
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