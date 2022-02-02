package com.cxp.nativelibffmpeg.utils

import java.nio.ByteBuffer
import java.util.*

// pcm对象缓冲
class PcmConsumerBuffer {
    private val freeBufferQueen = LinkedList<ByteArray>()
    val workAbleQueen = LinkedList<ByteArray>()
    fun recoveryByteWrap(byte: ByteArray) {
        freeBufferQueen.add(byte)
    }

    fun popWorkablePCM(): ByteArray? {
        return workAbleQueen.poll()
    }

    fun addPCMQueen(buffer: ByteBuffer, size: Int) {
        var csd: ByteArray? = null
        csd = freeBufferQueen.poll() ?: ByteArray(size)
        buffer.limit(0 + size)
        buffer.position(0)
        buffer.get(csd)
        workAbleQueen.add(csd)
    }
}