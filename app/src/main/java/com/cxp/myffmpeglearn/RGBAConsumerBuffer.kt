package com.cxp.myffmpeglearn

import android.graphics.Bitmap
import android.media.Image
import android.os.Build
import android.util.Log
import android.widget.ImageView
import androidx.annotation.RequiresApi

import java.nio.ByteBuffer
import java.util.*
import java.util.concurrent.LinkedBlockingDeque
import android.R.attr.scaleWidth
import android.graphics.Matrix


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



    fun rgba2nv12(): ByteArray? {
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

                    nv21!![uvIndex++] = (if (u > 0xFF) 0xFF else if (u < 0) 0 else u).toByte()
                    nv21!![uvIndex++] = (if (v > 0xFF) 0xFF else if (v < 0) 0 else v).toByte()
                }
            }
            argbIndex += rowPadding    //跳过rowPadding长度的填充数据
        }
        recoveryByteWrap(rgbaBuffer)
        return nv21
    }
}