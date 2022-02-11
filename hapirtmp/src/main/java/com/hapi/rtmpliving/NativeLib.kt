package com.hapi.rtmpliving

class NativeLib {

    /**
     * A native method that is implemented by the 'rtmpliving' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'rtmpliving' library on application startup.
        init {
            System.loadLibrary("rtmpliving")
        }
    }
}