package com.hapi.rtmpliving

class RTMPPusherContext {

    private var mNativeContextHandle: Long = 0


    external fun native_CreateContext()

    external fun native_DestroyContext()


    external fun native_startPush(
        outUrl: String,
        frameWidth: Int,
        frameHeight: Int,
        videoBitRate: Int,
        fps: Int,
        audioSampleRate: Int,
        audioChannelCount: Int,
        audioSampleFormat: Int
    ): Int

    external fun native_OnAudioData(
        data: ByteArray
    )

    external fun native_OnVideoData(
        format: Int,
        data: ByteArray,
        width: Int,
        height: Int
    )

    external fun native_OnVideoDataRgba(
        data: ByteArray,
        width: Int,
        height: Int,
        pixelStride: Int, rowPadding: Int
    )

    external fun native_StopRecord()


    companion object {
        val IMAGE_FORMAT_NV21 = 0x02
        val IMAGE_FORMAT_NV12 = 0x03
        val IMAGE_FORMAT_I420 = 0x04

        // Used to load the 'nativelibffmpeg' library on application startup.
        init {
            System.loadLibrary("rtmpliving")
        }
    }
}