package com.mjl.ffplay;

import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MyGLRender extends OpenGLVideoRender implements GLSurfaceView.Renderer,MyGLSurfaceView.OnGestureCallback {

    static {
        System.loadLibrary("ffplay");
    }
    public MyGLRender() {
        renderHandler = native_create();
    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        native_OnSurfaceCreated(renderHandler);
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int i, int i1) {
        native_OnSurfaceChanged(renderHandler, i, i1);
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        native_OnDrawFrame(renderHandler);
    }

    @Override
    public void onGesture(int xRotateAngle, int yRotateAngle, float scale) {
        native_SetGesture(renderHandler, xRotateAngle, yRotateAngle, scale);
    }

    @Override
    public void onTouchLoc(float touchX, float touchY) {
        native_SetTouchLoc(renderHandler, touchX, touchY);
    }
}
