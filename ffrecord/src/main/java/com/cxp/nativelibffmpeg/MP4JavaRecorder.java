package com.cxp.nativelibffmpeg;

import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;

import androidx.annotation.RequiresApi;

import com.cxp.nativelibffmpeg.service.AACEncoder;
import com.cxp.nativelibffmpeg.service.AudioFrameListener;
import com.cxp.nativelibffmpeg.service.H264Encoder;
import com.cxp.nativelibffmpeg.service.ImageListener;
import com.cxp.nativelibffmpeg.service.Mp4Muxer;
import com.cxp.nativelibffmpeg.service.RecordParams;

import java.io.File;
import java.nio.ByteBuffer;

/**
 * mp4录制
 */
public class MP4JavaRecorder implements ImageListener, AudioFrameListener {
    //h264编码
    private H264Encoder mAvEncoder;
    //mp4编码
    private Mp4Muxer mp4Muxer;
    private String filePath = "";
    private final RecordParams recordParams;
    //aac编码
    private AACEncoder audioEncode;
    public volatile boolean isStart = false;
    private Handler mVideoDecoderHandler = null;

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public MP4JavaRecorder(RecordParams recordParams) {
        this.recordParams = recordParams;
        //异步编码线程
        HandlerThread mVideoDecoderHandlerThread = new HandlerThread("VideoDecoder");
        mVideoDecoderHandlerThread.start();
        mVideoDecoderHandler = new Handler(mVideoDecoderHandlerThread.getLooper());
        createMediaRecorder();
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    private void createMediaRecorder() {
        createfile();
        int width = recordParams.width;
        int height = recordParams.height;

        mp4Muxer = new Mp4Muxer(filePath);
        mAvEncoder = new H264Encoder(mp4Muxer, width, height, recordParams.frameRate, recordParams.bitrate);
        mAvEncoder.mVideoDecoderHandler = mVideoDecoderHandler;

        audioEncode = new AACEncoder();
        audioEncode.init(mp4Muxer, recordParams.bitsPerSample, recordParams.sampleRate, recordParams.numberOfChannels);
        audioEncode.mVideoDecoderHandler = mVideoDecoderHandler;
    }

    private void createfile() {
        filePath = recordParams.filePath;
        File file = new File(filePath);
        try {
            if (!file.exists()) {
                file.createNewFile();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public void start() {
        isStart = true;
        mAvEncoder.start();
        audioEncode.start();
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public void stop() {
        isStart = false;
        if (audioEncode != null) {
            audioEncode.stop();
            audioEncode = null;
        }
        if (mAvEncoder != null) {
            mAvEncoder.stop();
        }
        if (mp4Muxer != null) {
            mp4Muxer.stop();
        }
    }

    // 接受音频数据
    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public void onAudioFrame(ByteBuffer buffer, int size,
                             int bitsPerSample,
                             int sampleRate,
                             int numberOfChannels) {
        if (!isStart) {
            return;
        }
        if (audioEncode != null) {
            audioEncode.putData(buffer, size);
        }
    }

    //接收屏幕rgba数据
    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    @Override
    public void onImageAvailable(byte[] rgbaBuffer, final int width, final int height, int pixelStride, int rowPadding) {
        if (!isStart) {
            return;
        }
        if (mAvEncoder == null) {
            return;
        }
        mAvEncoder.onImageAvailable(rgbaBuffer, width, height, pixelStride, rowPadding);
    }
}
