package com.cxp.nativelibffmpeg;

import android.os.Build;

import androidx.annotation.RequiresApi;

import com.cxp.nativelibffmpeg.service.AudioFrameListener;
import com.cxp.nativelibffmpeg.service.ImageListener;
import com.cxp.nativelibffmpeg.service.RecordParams;
import com.cxp.nativelibffmpeg.utils.PcmConsumerBuffer;
import com.cxp.nativelibffmpeg.utils.RGBAConsumerBuffer;

import java.nio.ByteBuffer;

public class VideoNdkRecorder implements ImageListener, AudioFrameListener {

    private final PcmConsumerBuffer consumerBuffer = new PcmConsumerBuffer();
    private final RGBAConsumerBuffer rgbaConsumerBuffer = new RGBAConsumerBuffer();
    private MediaRecorderContext mediaRecorderContext = null;
    private final RecordParams recordParams;
    public VideoNdkRecorder(RecordParams recordParams) {
        mediaRecorderContext = new MediaRecorderContext();
        mediaRecorderContext.native_CreateContext();
        this.recordParams = recordParams;
    }

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    @Override
    public void onImageAvailable(byte[] rgbaBuffer, int width, int height, int pixelStride, int rowPadding) {
        //  rgbaConsumerBuffer.addRgbQueen(rgbaBuffer, width, height, pixelStride, rowPadding);
        mediaRecorderContext.native_OnVideoDataRgba(
                //  rgbaConsumerBuffer.rgba2NV21(),
                rgbaBuffer,
                width, height,
                pixelStride, rowPadding
        );
    }

    public void onAudioFrame(ByteBuffer buffer, int size,
                             int bitsPerSample,
                             int sampleRate,
                             int numberOfChannels) {

        consumerBuffer.addPCMQueen(buffer, size);
        byte[] pcm = consumerBuffer.popWorkablePCM();
        assert pcm != null;
        mediaRecorderContext.native_OnAudioData(pcm);
        consumerBuffer.recoveryByteWrap(pcm);
    }

    public void start() {
        mediaRecorderContext.native_StartRecord(MediaRecorderContext.Companion.getRECORDER_TYPE_AV(),
                recordParams.filePath,
                recordParams.width,
                recordParams.height,
                recordParams.bitrate,
                recordParams.frameRate,
                recordParams.sampleRate, recordParams.numberOfChannels, recordParams.bitsPerSample
        );
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public void stop() {
        mediaRecorderContext.native_StopRecord();
        mediaRecorderContext.native_DestroyContext();
    }
}
