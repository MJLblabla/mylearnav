package com.blabla.beauty

public class BeautyRender {
    companion object {
        init {
            System.loadLibrary("beauty")
        }
        val dynimic=1;
        val type_lut = 2;
    }

    protected var nativeHandler = 0L;

    private var lastBeautyType = -1;

    fun create(beautyType: Int) {
        if (lastBeautyType != -1) {
            release()
        }
        nativeHandler = native_create(beautyType)
        lastBeautyType = beautyType;
    }

    fun rendRGBAFrame(rgba: ByteArray, width: Int, height: Int) {
        native_rendRGBAFrame(lastBeautyType, nativeHandler, rgba, width, height)
    }

    fun setLUTTexure(lutTextureId:Int){
        native_setLUTTexure(lastBeautyType,nativeHandler,lutTextureId)
    }
    fun release() {
        native_release(lastBeautyType, nativeHandler)
        lastBeautyType = -1;
    }

    external fun native_create(beautyType: Int): Long

    external fun native_setLUTTexure(beautyType: Int,  nativeHandler: Long,lutTextureId:Int)

    external fun native_rendRGBAFrame(
        beautyType: Int,
        nativeHandler: Long,
        rgba: ByteArray, width: Int, height: Int
    )

    external fun native_release(beautyType: Int, nativeHandler: Long)


}


