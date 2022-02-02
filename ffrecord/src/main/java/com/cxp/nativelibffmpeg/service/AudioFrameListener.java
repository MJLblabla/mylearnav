package com.cxp.nativelibffmpeg.service;

import java.nio.ByteBuffer;

public interface AudioFrameListener {

    public void onAudioFrame(ByteBuffer buffer, int size,
                             int bitsPerSample,
                             int sampleRate,
                             int numberOfChannels);
}
