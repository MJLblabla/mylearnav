package com.cxp.nativelibffmpeg.utils

import android.graphics.Bitmap
import android.graphics.Matrix
import android.os.Build
import androidx.annotation.RequiresApi
import com.cxp.nativelibffmpeg.YuvUtil

import java.nio.ByteBuffer
import java.util.*
import java.util.concurrent.LinkedBlockingDeque


// rgba byte对象缓冲池
class RGBAConsumerBuffer {

    private val freeBufferQueen = LinkedBlockingDeque<ByteArray>()
    val workAbleQueen = LinkedBlockingDeque<ByteArray>()

    private var rowPadding = 0
    private var pixelStride = 0

    var mWidth = 0
    var mHeight = 0

    //回收空闲对象
    private fun recoveryByteWrap(byte: ByteArray) {
        //   Log.d("RGBAConsumerBuffer", "recoveryByteWrap " + freeBufferQueen.size)
        freeBufferQueen.add(byte)
    }

    //生产者生产了待消费的 rgba对象 -> 拷贝到待消费的队列里
    @RequiresApi(Build.VERSION_CODES.KITKAT)
    fun addRgbQueen(
        rgbaOriginBuffer: ByteArray,
        width: Int,
        height: Int,
        pixelStride: Int,
        rowPadding: Int
    ) {

        this.pixelStride = pixelStride
        this.rowPadding = rowPadding
        mWidth = width
        mHeight = height
        // 从空闲对象队列中取一个用来缓冲这个byte
        var rgbaBuffer = freeBufferQueen.poll()
        if (rgbaBuffer == null) {
            rgbaBuffer = ByteArray(rgbaOriginBuffer.size)
        }
        // 拷贝待消费的对象
        System.arraycopy(rgbaOriginBuffer, 0, rgbaBuffer, 0, rgbaOriginBuffer.size);
        workAbleQueen.add(rgbaBuffer)
    }

    //缓冲对象 防止频繁分配内存
    private var nv21: ByteArray? = null

    // 从待消费列表取出一个rgba 并转换成nv21
    fun rgba2NV21(): ByteArray? {
        //    Log.d("RGBAConsumerBuffer", "rgba2NV21 消费 workAbleQueen" + workAbleQueen.size)
        val rgbaBuffer = workAbleQueen.poll() ?: return null
        var yIndex = 0
        var uvIndex = mWidth * mHeight
        var argbIndex = 0
        if (nv21 == null) {
            nv21 = ByteArray(mWidth * mHeight * 3 / 2)
        }

        for (j in 0 until mHeight) {
            for (i in 0 until mWidth) {
                var r: Int = rgbaBuffer[argbIndex++].toInt()
                var g: Int = rgbaBuffer[argbIndex++].toInt()
                var b: Int = rgbaBuffer[argbIndex++].toInt()
                argbIndex++ //跳过a
                r = r and 0x000000FF
                g = g and 0x000000FF
                b = b and 0x000000FF
                val y = (66 * r + 129 * g + 25 * b + 128 shr 8) + 16
                nv21!![yIndex++] = (if (y > 0xFF) 0xFF else if (y < 0) 0 else y).toByte()
                if (j and 1 == 0 && argbIndex shr 2 and 1 == 0 && uvIndex < nv21!!.size - 2) {
                    val u = (-38 * r - 74 * g + 112 * b + 128 shr 8) + 128
                    val v = (112 * r - 94 * g - 18 * b + 128 shr 8) + 128
                    nv21!![uvIndex++] = (if (v > 0xFF) 0xFF else if (v < 0) 0 else v).toByte()
                    nv21!![uvIndex++] = (if (u > 0xFF) 0xFF else if (u < 0) 0 else u).toByte()
                }
            }
            argbIndex += rowPadding    //跳过rowPadding长度的填充数据
        }
        recoveryByteWrap(rgbaBuffer)
        return nv21
    }

    private var nv12Tmp: ByteArray? = null
    private var i420: ByteArray? = null
    private var i420Scale: ByteArray? = null
    // 从待消费列表取出一个rgba 并转换成nv12


    private fun getPhoneBrand(): String {
        val manufacturer = Build.MANUFACTURER
        return if (manufacturer != null && manufacturer.length > 0) {
            manufacturer.toLowerCase()
        } else {
            "unknown"
        }
    }

    private fun isHuaWei(): Boolean {
        val phoneBrand: String = getPhoneBrand()
        return (phoneBrand.contains("HUAWEI") || phoneBrand.contains("OCE")
                || phoneBrand.contains("huawei") || phoneBrand.contains("honor"))
    }

    var originBitmap: Bitmap? = null
    var argbTemp:IntArray?=null
    fun rgba2nv12(newWidth: Int, newHeight: Int): ByteArray? {
        if (isHuaWei()) {
     //   if (true) {
            val rgbaBuffer = workAbleQueen.poll() ?: return null
            if (originBitmap == null) {
                originBitmap = Bitmap.createBitmap(
                    mWidth + rowPadding / pixelStride,
                    mHeight,
                    Bitmap.Config.ARGB_8888
                )
            }
            originBitmap!!.copyPixelsFromBuffer(ByteBuffer.wrap(rgbaBuffer, 0, rgbaBuffer.size))
            val matrix = Matrix()
            matrix.postScale(newWidth/mWidth.toFloat(), newHeight/mHeight.toFloat())
            val newBm = Bitmap.createBitmap(originBitmap!!,0,0,mWidth,mHeight,matrix,false)

            if (nv21 == null) {
                nv21 = ByteArray(newWidth * newHeight * 3 / 2)
            }
            if(argbTemp==null){
                argbTemp= IntArray(newWidth * newHeight)
            }
            newBm.getPixels(argbTemp, 0, newWidth, 0, 0, newWidth, newHeight)
            newBm.recycle()
            YUVJavaUtil.rgbEncodeYUV420SP(nv21,argbTemp,newWidth,newHeight)

            if (i420Scale == null) {
                i420Scale = ByteArray(newWidth * newHeight * 3 / 2)
            }

            YUVJavaUtil.nv21ToI420(i420Scale, nv21, newWidth, newHeight)

            if (nv12Tmp == null) {
                nv12Tmp = ByteArray(newWidth * newHeight * 3 / 2)
            }
            YUVJavaUtil.I420ToNV12(i420Scale, nv12Tmp, newWidth, newHeight)
            recoveryByteWrap(rgbaBuffer)
            return nv12Tmp

        } else {
            val nv21 = rgba2NV21() ?: return null
            if (i420 == null) {
                i420 = ByteArray(nv21!!.size)
            }
            // YuvUtil.CropI420(nv21,mWidth,mHeight,i420,mWidth,mHeight,0,0)
            YUVJavaUtil.nv21ToI420(i420, nv21, mWidth, mHeight)
            if (i420Scale == null) {
                i420Scale = ByteArray(newWidth * newHeight * 3 / 2)
            }
            YuvUtil.ScaleI420(i420, mWidth, mHeight, i420Scale, newWidth, newHeight, 3)
            if (nv12Tmp == null) {
                nv12Tmp = ByteArray(newWidth * newHeight * 3 / 2)
            }
            YUVJavaUtil.I420ToNV12(i420Scale, nv12Tmp, newWidth, newHeight)
            return nv12Tmp
        }
    }
}