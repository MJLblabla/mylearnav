package com.mjl.ffplay;

public class NativeLib {

    // Used to load the 'ffplay' library on application startup.
    static {
        System.loadLibrary("ffplay");
    }

    /**
     * A native method that is implemented by the 'ffplay' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}