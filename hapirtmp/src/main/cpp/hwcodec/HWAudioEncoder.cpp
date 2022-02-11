
#include "HWAudioEncoder.h"


int HWAudioEncoder::start(RTMPPush *mRTMPPush, RecorderParam *param) {

    int sampleRate = param->audioSampleRate;
    int channelCount = param->channelCount;
    int bitrate = 64000;
    baseTime = sampleRate * param->sampleDeep * channelCount / 8;
    startTime = GetSysCurrentTimeNS();
    int inited_ = -1;

    do {
        const char *mine = "audio/mp4a-latm";
        media_codec_ = AMediaCodec_createEncoderByType(mine);
        media_format_ = AMediaFormat_new();
        AMediaFormat_setString(media_format_, "mime", mine);

        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_BIT_RATE, bitrate);
        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_AAC_PROFILE, 2);
        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_CHANNEL_COUNT, channelCount);//s
        AMediaFormat_setInt32(media_format_, AMEDIAFORMAT_KEY_SAMPLE_RATE, sampleRate);

        media_status_t status = AMEDIA_OK;

        if ((status = AMediaCodec_configure(media_codec_, media_format_, NULL, NULL,
                                            AMEDIACODEC_CONFIGURE_FLAG_ENCODE))) {
            LOGCATE("%s %d HWAudioEncoder fail (%d)", __FUNCTION__, __LINE__, status);
            break;
        }

        LOGCATE("%s %d HWAudioEncoder status: %d %s", __FUNCTION__, __LINE__, status,
                AMediaFormat_toString(media_format_));

        if ((status = AMediaCodec_start(media_codec_))) {
            LOGCATE("%s %d HWAudioEncoder fail (%d)", __FUNCTION__, __LINE__, status);
            break;
        }
        LOGCATE("%s %d HWAudioEncoder  start (%d)", __FUNCTION__, __LINE__, status);
        inited_ = 1;
        m_Exit = false;
    } while (false);

    return inited_;
}


void HWAudioEncoder::stop() {
    AMediaCodec_flush(media_codec_);
    m_Exit = true;
}

void HWAudioEncoder::clear() {
    if (media_format_) {
        AMediaFormat_delete(media_format_);
        media_format_ = nullptr;
    }

    if (media_codec_) {
        AMediaCodec_stop(media_codec_);
        AMediaCodec_delete(media_codec_);
        media_codec_ = nullptr;
    }
    while (!m_AudioFrameQueue.Empty()) {
        AudioFrame *frame = m_AudioFrameQueue.Pop();
        if (frame != nullptr) {
            delete frame;
            frame = nullptr;
        }
    }
}

long HWAudioEncoder::getTimestamp() {
    return ((m_SamplesCount * 1000000.0 / baseTime));
}

int HWAudioEncoder::dealOneFrame(RTMPPush *mRTMPPush) {
    int result = 0;
    if (m_Exit) {
        return -1;
    }
    while (m_AudioFrameQueue.Empty() && !m_Exit) {
        usleep(10 * 1000);
    }

    AudioFrame *frame = m_AudioFrameQueue.Pop();
    if (frame == nullptr) {
        return -1;
    }

    encodeFrame(frame->data, frame->dataSize, getTimestamp());

    recvFrame(mRTMPPush);
    EXIT:
    delete frame;
    frame = nullptr;

    return result;
}


bool HWAudioEncoder::encodeFrame(void *data, int size, int64_t pts) {
    LOGCATE("%s %d HWEncoder_dequeueInputBuffer input audio   pts %ld", __FUNCTION__, __LINE__,
            pts);
    ssize_t bufidx = AMediaCodec_dequeueInputBuffer(media_codec_, MEDIACODEC_TIMEOUT_USEC);
    if (bufidx < 0) {
        return false;
    }
    if (!data) {
        AMediaCodec_queueInputBuffer(media_codec_, bufidx, 0, 0, pts,
                                     AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
        return true;
    }
    size_t bufsize = 0;
    uint8_t *buf = AMediaCodec_getInputBuffer(media_codec_, bufidx, &bufsize);
    if (!buf) {
        LOGCATE("%s %d AMediaCodec_dequeueInputBuffer fail", __FUNCTION__, __LINE__);
        return false;
    }
    memcpy(buf, data, size);
    m_SamplesCount = m_SamplesCount + size;
    media_status_t status = AMediaCodec_queueInputBuffer(media_codec_, bufidx, 0, size, pts, 0);
    LOGCATE("%s %d AMediaCodec_queueInputBuffer status (%d)", __FUNCTION__, __LINE__, status);
    return true;
}

void HWAudioEncoder::flush(RTMPPush *mRTMPPush) {
    encodeFrame(nullptr, 0, 0);
    recvFrame(mRTMPPush);
}

void HWAudioEncoder::recvFrame(RTMPPush *mRTMPPush) {
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
                return;
            }
            uint8_t *encodeData = AMediaCodec_getOutputBuffer(media_codec_, status,
                                                              NULL/* out_size */);
            if ((info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) != 0) {
                LOGCATE("ignoring AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG");
                info.size = 0;
            }
            size_t dataSize = info.size;

            LOGCATE("nalu, AMediaCodec_dequeueOutputBuffer audio type: %d size: %u flags: %u offset: %u pts: %ld",
                    1, info.size, info.flags, info.offset, info.presentationTimeUs);
            int type = encodeData[4] & 0x1f;
            mRTMPPush->pushAudioData(encodeData, info.size, type);
            AMediaCodec_releaseOutputBuffer(media_codec_, status, false);
            if ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) {
                break;
            }
        }
    }

}