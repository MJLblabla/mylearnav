package com.cxp.nativelibffmpeg.service;

import android.annotation.SuppressLint;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Handler;

import androidx.annotation.RequiresApi;

import com.cxp.nativelibffmpeg.utils.NV12ConsumerBuffer;
import com.cxp.nativelibffmpeg.utils.RGBAConsumerBuffer;

import java.io.IOException;
import java.nio.ByteBuffer;

public class H264Encoder {

    public MediaCodec mediaCodec;
    int m_width;
    int m_height;
    private final RGBAConsumerBuffer rgbaConsumerBuffer = new RGBAConsumerBuffer();
    private final NV12ConsumerBuffer nv12ConsumerBuffer = new NV12ConsumerBuffer();

    private volatile boolean isStop = false;
    public Handler mVideoDecoderHandler = null;
    private Mp4Muxer mMp4Muxer;
    @SuppressLint("NewApi")
    public H264Encoder( Mp4Muxer mp4Muxer,int width, int height, int framerate, int bitrate) {
        mMp4Muxer=mp4Muxer;
        m_width = width;
        m_height = height;
        try {
            mediaCodec = MediaCodec.createEncoderByType("video/avc");
        } catch (IOException e) {
            e.printStackTrace();
        }
        MediaFormat mediaFormat = MediaFormat.createVideoFormat("video/avc", width, height);
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitrate);
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, framerate);
        //  mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar);
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);//关键帧间隔时间 单位s
        //   mediaFormat.setInteger(MediaFormat.KEY_BITRATE_MODE, MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR);
        mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        //  this.surface = mediaCodec.createInputSurface();
        // mediaCodec.start();
        mp4Muxer.addVideoTrack(mediaCodec.getOutputFormat());
    }


    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    public void onImageAvailable(byte[] rgbaBuffer, final int width, final int height, int pixelStride, int rowPadding) {

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && mVideoDecoderHandler != null) {
            if (rgbaConsumerBuffer.getWorkAbleQueen().size() > 5) {
                return;
            }
            rgbaConsumerBuffer.addRgbQueen(rgbaBuffer, width, height, pixelStride, rowPadding);
        } else {
            if (rgbaConsumerBuffer.getWorkAbleQueen().size() > 10) {
                return;
            }

            rgbaConsumerBuffer.addRgbQueen(rgbaBuffer, width, height, pixelStride, rowPadding);
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public void start() {
        MediaCodec.Callback mMediaCodecCallback = new MediaCodec.Callback() {
            @Override
            public void onInputBufferAvailable(MediaCodec codec, final int id) {

                if (isStop) {
                    return;
                }
                ByteBuffer inputBuffer = mediaCodec.getInputBuffer(id);
                inputBuffer.clear();
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && mVideoDecoderHandler != null) {
                    byte[] dataSources = rgbaConsumerBuffer.rgba2nv12(m_width, m_height);
                    int length = 0;

                    if (isStop) {
                        return;
                    }
                    if (dataSources != null) {
                        length = dataSources.length;
                        inputBuffer.put(dataSources);
                    }
                    mediaCodec.queueInputBuffer(id, 0, length, getPTSUs(), 0);
                } else {
                    byte[] dataSources = nv12ConsumerBuffer.popWorkable();
                    int length = 0;
                    if (isStop) {
                        return;
                    }
                    if (dataSources != null) {
                        length = dataSources.length;
                        inputBuffer.put(dataSources);
                    }
                    mediaCodec.queueInputBuffer(id, 0, length, getPTSUs(), 0);
                    if (dataSources != null) {
                        nv12ConsumerBuffer.recoveryByteWrap(dataSources);
                    }
                }
            }

            @Override
            public void onOutputBufferAvailable(MediaCodec codec, int index, MediaCodec.BufferInfo info) {
                if (isStop) {
                    return;
                }
                ByteBuffer buffer = mediaCodec.getOutputBuffer(index);
                //  byte[] h264 = encodeFrame(buffer, info.size);
                mMp4Muxer.writeVideoData(buffer,info);
                mediaCodec.releaseOutputBuffer(index, false);
            }

            @Override
            public void onError(MediaCodec codec, MediaCodec.CodecException e) {
                e.printStackTrace();
            }

            @Override
            public void onOutputFormatChanged(MediaCodec codec, MediaFormat format) {
            }
        };

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && mVideoDecoderHandler != null) {
            mediaCodec.setCallback(mMediaCodecCallback, mVideoDecoderHandler);
        } else {
            mediaCodec.setCallback(mMediaCodecCallback);
            new Thread(new Runnable() {
                @Override
                public void run() {
                    while (!isStop) {
                        byte[] nv12 = rgbaConsumerBuffer.rgba2nv12(m_width, m_height);
                        if (nv12 == null) {
                            try {
                                Thread.sleep(1000 / 20);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        } else {
                            nv12ConsumerBuffer.addQueen(nv12);
                        }
                    }
                }
            }).start();
        }
        mediaCodec.start();
    }

    public void stop() {
        isStop = true;
        if (mediaCodec != null) {
            mediaCodec.stop();
        }
    }

    private long prevPresentationTimes = 0;
    private int count = 0;
    private long getPTSUs() {
        //  long result = System.nanoTime() / 1000;
        if (0 == prevPresentationTimes) {
            prevPresentationTimes = System.nanoTime();
        }
        long res= (System.nanoTime()  - prevPresentationTimes)/1000;
        count++;
        //Log.d("getPTSUs","视频 "+count+"   "+res);
        return res;
    }


}

