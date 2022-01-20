package com.mjl.ffplay;

public class OpenGLVideoRender {

    static {
        System.loadLibrary("ffplay");
    }
    protected long renderHandler = 0;

    public static native long native_create();
    //for GL render
    public static native void native_OnSurfaceCreated(long renderHandler);
    public static native void native_OnSurfaceChanged(long renderHandler, int width, int height);
    public static native void native_OnDrawFrame(long renderHandler);
    //update MVP matrix
    public static native void native_SetGesture(long renderHandler, float xRotateAngle, float yRotateAngle, float scale);
    public static native void native_SetTouchLoc(long renderHandler, float touchX, float touchY);

}
