package com.cxp.nativelibffmpeg

class MediaRecorderContext {

    private var mNativeContextHandle: Long = 0


    /**
     * A native method that is implemented by the 'nativelibffmpeg' native library,
     * which is packaged with this application.
     */
     external fun stringFromJNI(): String


     external fun native_CreateContext()

     external fun native_DestroyContext()



     external fun native_StartRecord(
        recorderType: Int,
        outUrl: String?,
        frameWidth: Int,
        frameHeight: Int,
        videoBitRate: Long,
        fps: Int
    ): Int

     external fun native_OnAudioData(data: ByteArray?)

     external fun native_OnVideoData(
        format: Int,
        data: ByteArray?,
        width: Int,
        height: Int
    )

     external fun native_StopRecord()



    
    companion object {
        val IMAGE_FORMAT_RGBA = 0x01
        val IMAGE_FORMAT_NV21 = 0x02
        val IMAGE_FORMAT_NV12 = 0x03
        val IMAGE_FORMAT_I420 = 0x04

        val RECORDER_TYPE_SINGLE_VIDEO = 0 //仅录制视频

        val RECORDER_TYPE_SINGLE_AUDIO = 1 //仅录制音频

        val RECORDER_TYPE_AV = 2 //同时录制音频和视频,打包成 MP4 文件
        // Used to load the 'nativelibffmpeg' library on application startup.
        init {
            System.loadLibrary("learn-ffmpeg")
        }
    }
}