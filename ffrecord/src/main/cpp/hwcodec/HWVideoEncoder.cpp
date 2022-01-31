#include "HWVideoEncoder.h"

int HWVideoEncoder::start(MP4Muxer *mMuxer, RecorderParam *param) {

    int width = param->frameWidth;
    int height = param->frameHeight;
    int fps = param->fps;
    int bitrate = param->videoBitRate;
    baseTime = 1000000L / param->fps;
    int inited_ = -1;

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
        startTime = GetSysCurrentTimeNS();
    } while (false);

    LOGCATE("%s %d inited: %s", __FUNCTION__, __LINE__, inited_ ? "success" : "false");
    return inited_;
}


void HWVideoEncoder::stop() {
    AMediaCodec_flush(media_codec_);
    m_Exit = true;
}

void HWVideoEncoder::clear() {

    if (media_format_) {
        AMediaFormat_delete(media_format_);
        media_format_ = nullptr;
    }

    if (media_codec_) {
        AMediaCodec_stop(media_codec_);
        AMediaCodec_delete(media_codec_);
        media_codec_ = nullptr;
    }

}

long HWVideoEncoder::getTimestamp() {
    return frame_idx_ * baseTime;
}

int HWVideoEncoder::dealOneFrame(MP4Muxer *mMuxer) {
    int result = 0;
    if (m_Exit) {
        return -1;
    }
    while (mVideoFrameQueue.Empty() && !m_Exit) {
        usleep(10 * 1000);
    }

    VideoFrame *videoFrame = mVideoFrameQueue.Pop();
    if (videoFrame == nullptr) {
        return -1;
    }
    long current = GetSysCurrentTimeNS();

    if (!encodeFrame(videoFrame->ppPlane[0], videoFrame->width * videoFrame->height * 3 / 2,
                     current -startTime)) {
        result = -1;
        goto EXIT;
    }
    recvFrame(mMuxer);
    EXIT:
    NativeImageUtil::FreeNativeImage(videoFrame);
    delete videoFrame;
    return result;
}


bool HWVideoEncoder::encodeFrame(void *data, int size, int64_t pts) {
    LOGCATE("%s %d HWEncoder_dequeueInputBuffer input video   pts %ld", __FUNCTION__, __LINE__,
            pts);
    ssize_t bufidx = AMediaCodec_dequeueInputBuffer(media_codec_, MEDIACODEC_TIMEOUT_USEC);
    if (bufidx < 0) {
        return false;
    }
    if (!data) {
        AMediaCodec_queueInputBuffer(media_codec_, bufidx, 0, 0, pts,
                                     AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
        LOGCATE("%s %d AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM: %d", __FUNCTION__, __LINE__,
                AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
        return true;
    }

    size_t bufsize = 0;
    uint8_t *buf = AMediaCodec_getInputBuffer(media_codec_, bufidx, &bufsize);
    if (!buf) {
        LOGCATE("%s %d AMediaCodec_dequeueInputBuffer fail", __FUNCTION__, __LINE__);
        return false;
    }

    LOGCATE("%s %d AMediaCodec_queueInputBuffer bfidx: %zd bfsize: %zu size: %d pts: %ld",
            __FUNCTION__, __LINE__, bufidx, bufsize, size, pts);
    memcpy(buf, data, size);

//    uint32_t flags = 0;
//    if (frame_idx_ % 5 == 0) {
//        flags |= AMEDIACODEC_CONFIGURE_FLAG_ENCODE;
//    } else if (frame_idx_ % 10 == 0) {
//        flags |= AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG;
//    }
    media_status_t status = AMediaCodec_queueInputBuffer(media_codec_, bufidx, 0, size, pts, 0);
    LOGCATE("%s %d AMediaCodec_queueInputBuffer status (%d)", __FUNCTION__, __LINE__, status);
    return true;
}

void HWVideoEncoder::recvFrame(MP4Muxer *mMuxer) {
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

            AMediaFormat *format = AMediaCodec_getOutputFormat(media_codec_);
            if (format) {
                uint8_t *sps = nullptr;
                size_t sps_len = 0;
                uint8_t *pps = nullptr;
                size_t pps_len = 0;
                bool sps_ok = AMediaFormat_getBuffer(format, "csd-0", (void **) &sps, &sps_len);
                bool pps_ok = AMediaFormat_getBuffer(format, "csd-1", (void **) &pps, &pps_len);
                if (sps_ok && pps_ok) {
                    int sps_type = sps[4] & 0x1f;
                    int pps_type = pps[4] & 0x1f;
                    LOGCATE("%s %d AMediaCodec_dequeueOutputBuffer AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED sps_type: %d sps_len: %u pps_type: %d pps_len: %u",
                            __FUNCTION__, __LINE__, sps_type, sps_len, pps_type, pps_len);


                    LOGCATE("pps_okpps_okpps_ok  \"%\" PRIu8 \"n\"   sps %p", pps, sps);
                }
                LOGCATE("%s %d AMediaCodec_dequeueOutputBuffer AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED %s",
                        __FUNCTION__, __LINE__, AMediaFormat_toString(format));
                mTrackIndex = mMuxer->addVideoTrack(format);
                AMediaFormat_delete(format);
            }
            continue;

        } else {
            if (status < 0) {
                return;
            }
            uint8_t *encodeData = AMediaCodec_getOutputBuffer(media_codec_, status,
                                                              NULL/* out_size */);
            int type = encodeData[4] & 0x1f;

            LOGCATE("nalu, AMediaCodec_dequeueOutputBuffer type: %d size: %u flags: %u offset: %u pts: %ld",
                    type, info.size, info.flags, info.offset, info.presentationTimeUs);

            if ((info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) != 0) {
                LOGCATE("ignoring AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG");
                info.size = 0;
            }
            size_t dataSize = info.size;
            ++frame_idx_;
            info.presentationTimeUs = getTimestamp();
            mMuxer->writeSampleData(mTrackIndex, encodeData, &info);
            AMediaCodec_releaseOutputBuffer(media_codec_, status, false);
            if ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) {
                break;
            }
        }
    }

}

void HWVideoEncoder::flush(MP4Muxer *mMuxer) {
    encodeFrame(nullptr, 0, 0);
    recvFrame(mMuxer);
}

bool HWVideoEncoder::isTrackAdded() {
    return mTrackIndex != -1;
}
