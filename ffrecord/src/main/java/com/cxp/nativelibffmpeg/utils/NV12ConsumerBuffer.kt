package com.cxp.nativelibffmpeg.utils

import java.util.concurrent.LinkedBlockingDeque

// nv12byte数组缓冲
class NV12ConsumerBuffer {

    val workAbleQueen = LinkedBlockingDeque<ByteArray>()
    val freeQueue = LinkedBlockingDeque<ByteArray>()

    fun popWorkable(): ByteArray? {
        return workAbleQueen.poll()
    }

    fun recoveryByteWrap(byte: ByteArray) {
        freeQueue.add(byte)
    }

    fun addQueen(byte: ByteArray) {
        val temp = freeQueue.poll() ?: ByteArray(byte.size)
        System.arraycopy(byte, 0, temp, 0, byte.size)
        workAbleQueen.add(temp)
    }
}