package com.cxp.nativelibffmpeg.service;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Handler;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import com.cxp.nativelibffmpeg.utils.PcmConsumerBuffer;

import java.io.IOException;
import java.nio.ByteBuffer;

public class AACEncoder {

    private MediaCodec encoder;
    private Mp4Muxer mediaMuxer;
    private boolean isRun;
    private PcmConsumerBuffer consumerBuffer = new PcmConsumerBuffer();
    public Handler mVideoDecoderHandler = null;

    public void init(Mp4Muxer mediaMuxer, int bitrateFormat, int sampleRate,
                     int channelCount) {
        this.mediaMuxer = mediaMuxer;
        MediaFormat audioFormat = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, sampleRate, channelCount);
        //设置比特率
        audioFormat.setInteger(MediaFormat.KEY_BIT_RATE, 96000);
        audioFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
        audioFormat.setInteger(MediaFormat.KEY_CHANNEL_COUNT, channelCount);
        audioFormat.setInteger(MediaFormat.KEY_SAMPLE_RATE, sampleRate);
        try {
            encoder = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
        } catch (IOException e) {
            e.printStackTrace();
        }
        encoder.configure(audioFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
    }

    public void putData(ByteBuffer buffer, int size) {
        if (!isRun) {
            return;
        }
        if (consumerBuffer.getWorkAbleQueen().size() > 3) {
            return;
        }
        consumerBuffer.addPCMQueen(buffer, size);
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public void start() {
        isRun = true;
        MediaCodec.Callback callback = new MediaCodec.Callback() {
            @Override
            public void onInputBufferAvailable(@NonNull MediaCodec mediaCodec, int id) {
                if(!isRun){
                    return;
                }
                ByteBuffer inputBuffer = mediaCodec.getInputBuffer(id);
                inputBuffer.clear();
                byte[] dataSources = consumerBuffer.popWorkablePCM();
                int length = 0;
                if (dataSources != null) {
                    length = dataSources.length;
                    inputBuffer.put(dataSources);
                }
                mediaCodec.queueInputBuffer(id, 0, length,System.nanoTime()/1000, 0);
                if (dataSources != null) {
                    consumerBuffer.recoveryByteWrap(dataSources);
                }
            }

            @Override
            public void onOutputBufferAvailable(@NonNull MediaCodec mediaCodec, int outputBufferindex, @NonNull MediaCodec.BufferInfo bufferInfo) {
                if(!isRun){
                    return;
                }
                ByteBuffer buffer = mediaCodec.getOutputBuffer(outputBufferindex);
                if (mediaMuxer.audioMediaFormat == null) {
                    mediaMuxer.addAudioTrack(encoder.getOutputFormat());
                }
                bufferInfo.presentationTimeUs=getPTSUs();
                mediaMuxer.writeAudioData(buffer, bufferInfo);
                mediaCodec.releaseOutputBuffer(outputBufferindex, false);
            }

            @Override
            public void onError(@NonNull MediaCodec mediaCodec, @NonNull MediaCodec.CodecException e) {
                e.printStackTrace();
            }

            @Override
            public void onOutputFormatChanged(@NonNull MediaCodec mediaCodec, @NonNull MediaFormat mediaFormat) {
            }
        };
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && mVideoDecoderHandler != null) {
            encoder.setCallback(callback, mVideoDecoderHandler);
        } else {
            encoder.setCallback(callback);
        }
        encoder.start();
    }

    public void stop() {
        isRun = false;
        encoder.stop();
        encoder.release();
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

        return res;
    }
}

