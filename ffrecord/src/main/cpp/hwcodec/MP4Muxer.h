

#ifndef MYFFMPEGLEARN_MP4MUXER_H
#define MYFFMPEGLEARN_MP4MUXER_H

#include <media/NdkMediaMuxer.h>

class MP4Muxer {

private:
    AMediaMuxer *mMuxer = nullptr;
    size_t videoTrackId = -1;
    size_t audioTrackId = -1;
    FILE *fp_ = nullptr;

public:
    MP4Muxer(char m_OutUrl[1024]) {
        fp_ = fopen(m_OutUrl, "wb");
        int result = 0;
        if (!(fp_ = fopen(m_OutUrl, "wb"))) {
            result = -1;
            //   return result;
        }
        int mFd = fileno(fp_);
        mMuxer = AMediaMuxer_new(mFd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
    }

    ~MP4Muxer() {
        AMediaMuxer_stop(mMuxer);
        AMediaMuxer_delete(mMuxer);
    }

    size_t addVideoTrack(AMediaFormat
                         *media_format) {
        videoTrackId = AMediaMuxer_addTrack(mMuxer, media_format);
        if (videoTrackId != -1 && audioTrackId != -1) {
            AMediaMuxer_start(mMuxer);
        }
        return videoTrackId;
    }

    size_t addAudioTrack(AMediaFormat
                         *media_format) {
        audioTrackId = AMediaMuxer_addTrack(mMuxer, media_format);
        if (videoTrackId != -1 && audioTrackId != -1) {
            AMediaMuxer_start(mMuxer);
        }
        return audioTrackId;
    }

    void writeSampleData(size_t trackIdx,
                         uint8_t *data,
                         AMediaCodecBufferInfo *info
    ) {
        if (videoTrackId != -1 && audioTrackId != -1) {
            LOGCATE("AMediaMuxer_writeSampleData  size %d trackIdx %zu pts %ld", info->size,trackIdx ,info->presentationTimeUs);
            AMediaMuxer_writeSampleData(mMuxer, trackIdx, data, info);
        }
    }

};

#endif