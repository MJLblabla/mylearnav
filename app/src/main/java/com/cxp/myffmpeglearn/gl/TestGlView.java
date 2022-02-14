package com.cxp.myffmpeglearn.gl;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;

import com.blabla.beauty.BeautyRender;
import com.cxp.myffmpeglearn.R;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class TestGlView extends GLSurfaceView {
    private static final String TAG = "MyGLSurfaceView";

    public TestGlView(Context context) {
        super(context);
    }

    public TestGlView(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.setEGLContextClientVersion(3);

        MyGLRender mGLRender = new MyGLRender(context);
        setRenderer(mGLRender);
        setRenderMode(RENDERMODE_CONTINUOUSLY);
    }


    public static class MyGLRender implements GLSurfaceView.Renderer {

        private BeautyRender mBeautyRender = new BeautyRender();
        private int width;
        private int height;
        private Context context;

        MyGLRender(Context context) {
            this.context = context;

        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            Log.d(TAG, "onSurfaceCreated() called with: gl = [" + gl + "], config = [" + config + "]");
            mBeautyRender.create(BeautyRender.Companion.getType_lut());
            //TextureUtils.init();
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            Log.d(TAG, "onSurfaceChanged() called with: gl = [" + gl + "], width = [" + width + "], height = [" + height + "]");
            this.width = width;
            this.height = height;

        }

        @Override
        public void onDrawFrame(GL10 gl) {
            Log.d(TAG, "onDrawFrame() called with: gl = [" + gl + "]");
            //  int id =TextureUtils.loadTexture(BitmapFactory.decodeResource(context.getResources(), R.drawable.bg_board));

//            Bitmap bitmap = BitmapFactory.decodeResource(context.getResources(), R.drawable.bg_board);
//             bitmap.
//            mBeautyRender.rendRGBAFrame(bitmap.re, width, height);

        }
    }
}
