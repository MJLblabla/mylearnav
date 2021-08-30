package com.cxp.myffmpeglearn;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import java.util.concurrent.Executors;

public class AudioRecorder {
    private static final String TAG = "AudioRecorder";
    private AudioRecord mAudioRecord = null;
    private static final int DEFAULT_SAMPLE_RATE = 44100;
    private static final int DEFAULT_CHANNEL_LAYOUT = AudioFormat.CHANNEL_IN_STEREO;
    private static final int DEFAULT_SAMPLE_FORMAT = AudioFormat.ENCODING_PCM_16BIT;
    private final AudioRecorderCallback mRecorderCallback;

    public AudioRecorder(AudioRecorderCallback callback) {
        this.mRecorderCallback = callback;
    }


    public void run() {

        final int mMinBufferSize = AudioRecord.getMinBufferSize(DEFAULT_SAMPLE_RATE, DEFAULT_CHANNEL_LAYOUT, DEFAULT_SAMPLE_FORMAT);
        Log.d(TAG, "run() called mMinBufferSize=" + mMinBufferSize);

        if (AudioRecord.ERROR_BAD_VALUE == mMinBufferSize) {
            mRecorderCallback.onError("parameters are not supported by the hardware.");
            return;
        }

        mAudioRecord = new AudioRecord(android.media.MediaRecorder.AudioSource.MIC, DEFAULT_SAMPLE_RATE, DEFAULT_CHANNEL_LAYOUT, DEFAULT_SAMPLE_FORMAT, mMinBufferSize);

        start();
    }


    private boolean isStart = true;


    private void start() {

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    mAudioRecord.startRecording();
                } catch (IllegalStateException e) {
                    mRecorderCallback.onError(e.getMessage() + " [startRecording failed]");
                    return;
                }
                byte[] sampleBuffer = new byte[4096];
                try {
                    while (isStart && mAudioRecord != null && !Thread.currentThread().isInterrupted()) {

                        int result = mAudioRecord.read(sampleBuffer, 0, 4096);
                        if (result > 0) {
                            mRecorderCallback.onAudioData(sampleBuffer, result);
                        }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    mRecorderCallback.onError(e.getMessage());
                }
            }
        }).start();
    }

    public void stop() {
        isStart = false;

        mAudioRecord.release();
        mAudioRecord = null;
    }

    public interface AudioRecorderCallback {
        void onAudioData(byte[] data, int dataSize);

        void onError(String msg);
    }
}
