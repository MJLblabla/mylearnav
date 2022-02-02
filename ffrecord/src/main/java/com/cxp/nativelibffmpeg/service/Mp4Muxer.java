package com.cxp.nativelibffmpeg.service;


import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import java.io.IOException;
import java.nio.ByteBuffer;

/**
 * 描述：mp4合成器
 */

public class Mp4Muxer {
    public static final boolean VERBOSE = true;
    private String TAG = getClass().getSimpleName();
    private boolean isStart = false;
    private MediaMuxer mMuxer;
    private int mVideoTrackIndex = -1, mAudioTrackIndex = -1;

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public Mp4Muxer(String outPath) {
        try {
            mMuxer = new MediaMuxer(outPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    MediaFormat videoMediaFormat;
    MediaFormat audioMediaFormat;

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public void addVideoTrack(MediaFormat mediaFormat) {
        this.videoMediaFormat = mediaFormat;
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public void addAudioTrack(MediaFormat mediaFormat) {
        audioMediaFormat = mediaFormat;
    }

    public boolean isStart() {
        return isStart;
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    synchronized
    public void writeVideoData(ByteBuffer outputBuffer, MediaCodec.BufferInfo bufferInfo) {
        if (mMuxer == null) {
            return;
        }
        if (mVideoTrackIndex == -1) {
            writeHeadInfo(videoMediaFormat, outputBuffer, bufferInfo);
        }
        writeData(outputBuffer, bufferInfo, mVideoTrackIndex);
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    private void writeHeadInfo(MediaFormat outputFormat, ByteBuffer outputBuffer, MediaCodec.BufferInfo bufferInfo) {
        byte[] csd = new byte[bufferInfo.size];
        outputBuffer.limit(bufferInfo.offset + bufferInfo.size);
        outputBuffer.position(bufferInfo.offset);
        outputBuffer.get(csd);
        ByteBuffer sps = null;
        ByteBuffer pps = null;
        for (int i = bufferInfo.size - 1; i > 3; i--) {
            if (csd[i] == 1 && csd[i - 1] == 0 && csd[i - 2] == 0 && csd[i - 3] == 0) {
                sps = ByteBuffer.allocate(i - 3);
                pps = ByteBuffer.allocate(bufferInfo.size - (i - 3));
                sps.put(csd, 0, i - 3).position(0);
                pps.put(csd, i - 3, bufferInfo.size - (i - 3)).position(0);
            }
        }

        if (sps != null && pps != null) {
            outputFormat.setByteBuffer("csd-0", sps);
            outputFormat.setByteBuffer("csd-1", pps);
        }
        mVideoTrackIndex = mMuxer.addTrack(outputFormat);
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    synchronized
    public void writeAudioData(ByteBuffer outputBuffer, MediaCodec.BufferInfo bufferInfo) {
        if (mMuxer == null) {
            return;
        }
        if (mAudioTrackIndex == -1) {
            mAudioTrackIndex = mMuxer.addTrack(audioMediaFormat);
        }
        writeData(outputBuffer, bufferInfo, mAudioTrackIndex);
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    void writeData(ByteBuffer outputBuffer, MediaCodec.BufferInfo bufferInfo, int track) {

        if (mAudioTrackIndex == -1 || mVideoTrackIndex == -1) {
            return;
        }
        if (!isStart) {
            isStart = true;
            mMuxer.start();
        }
        if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
            // The codec config data was pulled out and fed to the muxer when we got
            // the INFO_OUTPUT_FORMAT_CHANGED status.  Ignore it.
            bufferInfo.size = 0;
        } else if (bufferInfo.size != 0) {
            outputBuffer.position(bufferInfo.offset);
            outputBuffer.limit(bufferInfo.offset + bufferInfo.size);
            try {
                mMuxer.writeSampleData(track, outputBuffer, bufferInfo);
            } catch (Exception e) {
                e.printStackTrace();
            }
            if (VERBOSE)
                Log.d(TAG, String.format("send [%d] [" + bufferInfo.size + "] with timestamp:[%d] to muxer", track, bufferInfo.presentationTimeUs));
            if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) {
                if (VERBOSE)
                    Log.i(TAG, "BUFFER_FLAG_END_OF_STREAM received");
            }
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    synchronized
    public void stop() {
        isStart = false;
        if (mMuxer != null) {
            if (mVideoTrackIndex != -1 || mAudioTrackIndex != -1) {//mAudioTrackIndex != -1 &&
                if (VERBOSE)
                    Log.i(TAG, String.format("muxer is started. now it will be stoped."));
                try {
                    mMuxer.stop();
                    mMuxer.release();
                } catch (IllegalStateException ex) {
                    ex.printStackTrace();
                }
            }
        }
        mMuxer = null;
    }
}
