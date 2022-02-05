package com.mjl.ffplay;

public class FFPlayEngine {


    public static final int MSG_DECODER_INIT_ERROR      = 0;
    public static final int MSG_DECODER_READY           = 1;
    public static final int MSG_DECODER_DONE            = 2;
    public static final int MSG_REQUEST_RENDER          = 3;
    public static final int MSG_DECODING_TIME           = 4;

    static {
        System.loadLibrary("ffplay");
    }

    private EventCallback mEventCallback = null;

    private long mNativeEngineCtr = -2;

    public FFPlayEngine() {
        mNativeEngineCtr = nativeCreateFFPlayEngine();
    }

    public void destroy() {
        nativeDestroy(mNativeEngineCtr);
    }

    public void init() {
        nativeInit(mNativeEngineCtr);
    }

    public void setVideoPlayerRender(OpenGLVideoRender render) {
        nativeSetVideoPlayerRender(mNativeEngineCtr, render.renderHandler);
    }

    public void play(String url) {
        nativePlay(mNativeEngineCtr, url);
    }

    public void pause() {
        nativePause(mNativeEngineCtr);
    }

    public void stop() {
        nativeStop(mNativeEngineCtr);
    }

    public void seekToPosition(float position) {
        nativeSeekToPosition(mNativeEngineCtr, position);
    }

    public long getMediaParams(int paramType) {
        return nativeGetMediaParams(mNativeEngineCtr, paramType);
    }


    public interface EventCallback {
        void onPlayerEvent(int msgType, float msgValue);
    }

    private void playerEventCallback(int msgType, float msgValue) {
        if (mEventCallback != null)
            mEventCallback.onPlayerEvent(msgType, msgValue);
    }

    public void addEventCallback(EventCallback callback) {
        mEventCallback = callback;
    }


    public native  long nativeCreateFFPlayEngine();

    public native  void nativeDestroy(long nativeEngineCtr);

    public native  void nativeInit(long nativeEngineCtr);

    public native  void nativeSetVideoPlayerRender(long nativeEngineCtr, long render);

    public native  void nativePlay(long nativeEngineCtr,String url);

    public native  void nativePause(long nativeEngineCtr);

    public native  void nativeStop(long nativeEngineCtr);

    public native  void nativeSeekToPosition(long nativeEngineCtr, float position);

    public native  long nativeGetMediaParams(long nativeEngineCtr, int paramType);

}





