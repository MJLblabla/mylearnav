#include <HWEncoder.h>
#include "HWFFVideoEncoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
}
#define is_start_code(code)    (((code) & 0x0ffffff) == 0x01)
#define H264_NALU_TYPE_NON_IDR_PICTURE                                  1
#define H264_NALU_TYPE_IDR_PICTURE                                      5
#define H264_NALU_TYPE_SEQUENCE_PARAMETER_SET                           7
#define H264_NALU_TYPE_PICTURE_PARAMETER_SET                            8
#define H264_NALU_TYPE_SEI                                            6

int HWFFVideoEncoder::start(AVFormatContext *formatCtx, RecorderParam *param) {

    m_RecorderParam = *param;
    m_AVFormatContext = formatCtx;


    int width = param->frameWidth;
    int height = param->frameHeight;
    int fps = param->fps;
    int bitrate = param->videoBitRate;
    int inited_ = 0;
    do {
        const char *mine = "video/avc";
        media_codec_ = AMediaCodec_createEncoderByType(mine);
        media_format_ = AMediaFormat_new();
        AMediaFormat_setString(media_format_, "mime", mine);

        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_WIDTH, width); // 视频宽度
        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_HEIGHT, height); // 视频高度

        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_BIT_RATE, bitrate);
        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_FRAME_RATE, fps);
        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 1);//s
        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_COLOR_FORMAT, 19);
        // AMediaFormat_setInt32(media_format_, "bitrate-mode", MEDIACODEC_BITRATE_MODE_VBR);
        media_status_t status = AMEDIA_OK;

        if ((status = AMediaCodec_configure(media_codec_, media_format_, NULL, NULL,
                                            AMEDIACODEC_CONFIGURE_FLAG_ENCODE))) {
            LOGCATE("%s %d AMediaCodec_configure fail (%d)", __FUNCTION__, __LINE__, status);
            break;
        }
        LOGCATE("%s %d AMediaCodec_configure status: %d %s", __FUNCTION__, __LINE__, status,
                AMediaFormat_toString(media_format_));

        if ((status = AMediaCodec_start(media_codec_))) {
            LOGCATE("%s %d AMediaCodec_start fail (%d)", __FUNCTION__, __LINE__, status);
            break;
        }
//        mTrackIndex = AMediaMuxer_addTrack(mMuxer, media_format_);
        inited_ = 1;

    } while (false);
    if (inited_ == 0) {
        return -1;
    }
    /**
     * add stream
     */

    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        LOGCATE("SoftVideoEncoder::AddStream Could not find encoder for '%s'",
                avcodec_get_name(formatCtx->video_codec_id));
        return -1;
    }

    mAvStream = avformat_new_stream(formatCtx, codec);
    if (nullptr == mAvStream) {
        return -2;
    }
    mCodecCtx = avcodec_alloc_context3(nullptr);
    if (nullptr == mCodecCtx) {
        return -3;
    }

    mCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    mCodecCtx->codec_id = AV_CODEC_ID_H264;
    mCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    mCodecCtx->bit_rate = param->videoBitRate;
    mCodecCtx->width = param->frameWidth;
    mCodecCtx->height = param->frameHeight;
    AVRational video_time_base = {1, 10000};
    mCodecCtx->time_base = AVRational{1, param->fps};
//    mAvStream->avg_frame_rate = video_time_base;
    mAvStream->time_base = AVRational{1, 90000};
    mCodecCtx->gop_size = static_cast<int>(m_RecorderParam.fps);
    mCodecCtx->qmin = 10;
    mCodecCtx->qmax = 30;
    // 新增语句，设置为编码延迟
    av_opt_set(mCodecCtx->priv_data, "preset", "ultrafast", 0);
    // 实时编码关键看这句，上面那条无所谓
    av_opt_set(mCodecCtx->priv_data, "tune", "zerolatency", 0);
    if (m_AVFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        mCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    avcodec_parameters_from_context(mAvStream->codecpar, mCodecCtx);

    return 0;
}

void HWFFVideoEncoder::stop() {
    AMediaCodec_flush(media_codec_);
    m_Exit = true;
}

void HWFFVideoEncoder::clear() {


    if (media_format_) {
        AMediaFormat_delete(media_format_);
        media_format_ = nullptr;
    }

    if (media_codec_) {
        AMediaCodec_stop(media_codec_);
        AMediaCodec_delete(media_codec_);
        media_codec_ = nullptr;
    }


    av_frame_free(&mFrame);
    while (!mVideoFrameQueue.Empty()) {
        VideoFrame *pImage = mVideoFrameQueue.Pop();
        NativeImageUtil::FreeNativeImage(pImage);
        if (pImage) delete pImage;
    }
//    if (m_Packet) {
//        av_packet_free(&m_Packet);
//    }
    avcodec_free_context(&mCodecCtx);
    //delete mVideoCodec;
    mFrame = nullptr;
    m_Packet = nullptr;
}

int HWFFVideoEncoder::dealOneFrame() {

    LOGCATE("SoftVideoEncoder::EncodeVideoFrame %ld", mNextPts);
    int result = 0;

    if (m_Packet == nullptr) {
        m_Packet = av_packet_alloc();
    }

    while (mVideoFrameQueue.Empty() && !m_Exit) {
        usleep(10 * 1000);
    }

    AVPixelFormat srcPixFmt = AV_PIX_FMT_YUV420P;
    VideoFrame *videoFrame = mVideoFrameQueue.Pop();

    if (videoFrame == nullptr) {
        return -1;
    }
    write(videoFrame);

    while (!m_Exit) {
        AMediaCodecBufferInfo info;
        //time out usec 1
        ssize_t status = AMediaCodec_dequeueOutputBuffer(media_codec_, &info, 1);

        if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            break;
        } else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
            // not expected for an encoder
            continue;
        } else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            continue;
        } else {
            if (status < 0) {
                result = -1;
                goto EXIT;
            }

            int presentationTimeUs = info.presentationTimeUs;
            uint8_t *outputData = AMediaCodec_getOutputBuffer(media_codec_, status,
                                                              NULL/* out_size */);
            int nalu_type = (outputData[4] & 0x1F);

            int bufferSize = info.size;

            if (nalu_type == H264_NALU_TYPE_SEQUENCE_PARAMETER_SET) {
                // 我们这里要求sps和pps一块拼接起来构造成AVPacket传过来
                int header_size_ = bufferSize;
                uint8_t *header_data_ = new uint8_t[header_size_];

                memcpy(header_data_, outputData, static_cast<size_t>(bufferSize));

                uint8_t *spsFrame = 0;
                uint8_t *ppsFrame = 0;

                int spsFrameLen = 0;
                int ppsFrameLen = 0;

                ParseH264SequenceHeader(header_data_, static_cast<uint32_t>(header_size_),
                                        &spsFrame, spsFrameLen,
                                        &ppsFrame, ppsFrameLen);

                // Extradata contains PPS & SPS for AVCC format
                int extradata_len = 8 + spsFrameLen - 4 + 1 + 2 + ppsFrameLen - 4;


                mAvStream->codecpar->extradata = reinterpret_cast<uint8_t *>(av_mallocz(
                        static_cast<size_t>(extradata_len)));
                mAvStream->codecpar->extradata_size = extradata_len;
                mAvStream->codecpar->extradata[0] = 0x01;
                mAvStream->codecpar->extradata[1] = spsFrame[4 + 1];
                mAvStream->codecpar->extradata[2] = spsFrame[4 + 2];
                mAvStream->codecpar->extradata[3] = spsFrame[4 + 3];
                mAvStream->codecpar->extradata[4] = 0xFC | 3;
                mAvStream->codecpar->extradata[5] = 0xE0 | 1;
                int tmp = spsFrameLen - 4;
                mAvStream->codecpar->extradata[6] = static_cast<uint8_t>((tmp >> 8) & 0x00ff);
                mAvStream->codecpar->extradata[7] = static_cast<uint8_t>(tmp & 0x00ff);
                int i = 0;
                for (i = 0; i < tmp; i++) {
                    mAvStream->codecpar->extradata[8 + i] = spsFrame[4 + i];
                }
                mAvStream->codecpar->extradata[8 + tmp] = 0x01;
                int tmp2 = ppsFrameLen - 4;
                mAvStream->codecpar->extradata[8 + tmp + 1] = static_cast<uint8_t>((tmp2 >> 8) &
                                                                                   0x00ff);
                mAvStream->codecpar->extradata[8 + tmp + 2] = static_cast<uint8_t>(tmp2 & 0x00ff);
                for (i = 0; i < tmp2; i++) {
                    mAvStream->codecpar->extradata[8 + tmp + 3 + i] = ppsFrame[4 + i];
                }

                result = avformat_write_header(m_AVFormatContext, nullptr);

                delete[] header_data_;
                if (result < 0) {
                    LOGCATE("Error occurred when opening output file: %s\n", av_err2str(result));
                } else {
                    //   write_header_success_ = true;
                    LOGCATE("avformat_write_header success %d", result);
                }
            } else {

                int64_t pts = av_rescale_q(info.presentationTimeUs, AV_TIME_BASE_Q,
                                           mCodecCtx->time_base);
                if (nalu_type == H264_NALU_TYPE_IDR_PICTURE || nalu_type == H264_NALU_TYPE_SEI) {

                    m_Packet->size = info.size;
                    m_Packet->data = outputData;

                    if (m_Packet->data[0] == 0x00 && m_Packet->data[1] == 0x00 &&
                        m_Packet->data[2] == 0x00 && m_Packet->data[3] == 0x01) {
                        bufferSize -= 4;
                        m_Packet->data[0] = static_cast<uint8_t>(((bufferSize) >> 24) & 0x00ff);
                        m_Packet->data[1] = static_cast<uint8_t>(((bufferSize) >> 16) & 0x00ff);
                        m_Packet->data[2] = static_cast<uint8_t>(((bufferSize) >> 8) & 0x00ff);
                        m_Packet->data[3] = static_cast<uint8_t>(((bufferSize)) & 0x00ff);
                    }

                    m_Packet->pts = pts;
                    m_Packet->dts = pts;
                    m_Packet->flags = AV_PKT_FLAG_KEY;

                } else {
                    m_Packet->size = bufferSize;
                    m_Packet->data = outputData;

                    if (m_Packet->data[0] == 0x00 && m_Packet->data[1] == 0x00 &&
                        m_Packet->data[2] == 0x00 && m_Packet->data[3] == 0x01) {
                        bufferSize -= 4;
                        m_Packet->data[0] = static_cast<uint8_t>(((bufferSize) >> 24) & 0x00ff);
                        m_Packet->data[1] = static_cast<uint8_t>(((bufferSize) >> 16) & 0x00ff);
                        m_Packet->data[2] = static_cast<uint8_t>(((bufferSize) >> 8) & 0x00ff);
                        m_Packet->data[3] = static_cast<uint8_t>(((bufferSize)) & 0x00ff);
                    }

                    m_Packet->pts = pts;
                    m_Packet->dts = pts;
                    m_Packet->flags = 0;
                }
                m_Packet->stream_index = mAvStream->index;
                result = WritePacket(m_AVFormatContext, &mCodecCtx->time_base, mAvStream, m_Packet);
            }

            AMediaCodec_releaseOutputBuffer(media_codec_, status, false);
            if (result < 0) {
                LOGCATE("SoftVideoEncoder::EncodeVideoFrame video Error while writing audio frame: %s",
                        av_err2str(result));
                result = 0;
                goto EXIT;
            }
            if ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) {
                break;
            }
        }
    }

    EXIT:
    av_packet_unref(m_Packet);
    NativeImageUtil::FreeNativeImage(videoFrame);
    if (videoFrame) delete videoFrame;
    return result;
    return 0;

}


uint32_t HWFFVideoEncoder::FindStartCode(uint8_t *in_buffer, uint32_t in_ui32_buffer_size,
                                         uint32_t in_ui32_code,
                                         uint32_t &out_ui32_processed_bytes) {
    uint32_t ui32Code = in_ui32_code;
    const uint8_t *ptr = in_buffer;
    while (ptr < in_buffer + in_ui32_buffer_size) {
        ui32Code = *ptr++ + (ui32Code << 8);
        if (is_start_code(ui32Code)) {
            break;
        }
    }
    out_ui32_processed_bytes = (uint32_t) (ptr - in_buffer);
    return ui32Code;
}


void HWFFVideoEncoder::ParseH264SequenceHeader(uint8_t *in_buffer, uint32_t in_ui32_size,
                                               uint8_t **in_sps_buffer, int &in_sps_size,
                                               uint8_t **in_pps_buffer, int &in_pps_size) {
    uint32_t ui32StartCode = 0x0ff;
    uint8_t *pBuffer = in_buffer;
    uint32_t ui32BufferSize = in_ui32_size;
    uint32_t sps = 0;
    uint32_t pps = 0;
    uint32_t idr = in_ui32_size;
    do {
        uint32_t ui32ProcessedBytes = 0;
        ui32StartCode = FindStartCode(pBuffer, ui32BufferSize, ui32StartCode,
                                      ui32ProcessedBytes);
        pBuffer += ui32ProcessedBytes;
        ui32BufferSize -= ui32ProcessedBytes;
        if (ui32BufferSize < 1) {
            break;
        }

        uint8_t val = static_cast<uint8_t>(*pBuffer & 0x1f);
        // idr
        if (val == 5) {
            idr = pps + ui32ProcessedBytes - 4;
        }
        // sps
        if (val == 7) {
            sps = ui32ProcessedBytes;
        }
        // pps
        if (val == 8) {
            pps = sps + ui32ProcessedBytes;
        }
    } while (ui32BufferSize > 0);
    *in_sps_buffer = in_buffer + sps - 4;
    in_sps_size = pps - sps;
    *in_pps_buffer = in_buffer + pps - 4;
    in_pps_size = idr - pps + 4;
}


void HWFFVideoEncoder::write(VideoFrame *videoFrame) {
    uint8_t *data = videoFrame->ppPlane[0];
    mNextPts++;

    int64_t pts = av_rescale_q(mNextPts, mCodecCtx->time_base, AV_TIME_BASE_Q);

    int size = videoFrame->width * videoFrame->height * 3 / 2;

    ssize_t bufidx = AMediaCodec_dequeueInputBuffer(media_codec_, MEDIACODEC_TIMEOUT_USEC);
    if (bufidx < 0) {
        return;
    }
    if (!data) {
        AMediaCodec_queueInputBuffer(media_codec_, bufidx, 0, 0, pts,
                                     AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
        LOGCATE("%s %d AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM: %d", __FUNCTION__, __LINE__,
                AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
        return;
    }

    size_t bufsize = 0;
    uint8_t *buf = AMediaCodec_getInputBuffer(media_codec_, bufidx, &bufsize);
    if (!buf) {
        LOGCATE("%s %d AMediaCodec_dequeueInputBuffer fail", __FUNCTION__, __LINE__);
        return;
    }

    LOGCATE("%s %d AMediaCodec_queueInputBuffer bfidx: %zd bfsize: %zu size: %d pts: %ld",
            __FUNCTION__, __LINE__, bufidx, bufsize, size, pts);
    memcpy(buf, data, size);

    media_status_t status = AMediaCodec_queueInputBuffer(media_codec_, bufidx, 0, size, pts, 0);

}


AVRational HWFFVideoEncoder::getTimeBase() {
    return mCodecCtx->time_base;
}

