package com.cxp.nativelibffmpeg.service;

import android.content.Intent;
import android.os.Parcel;
import android.os.Parcelable;

public class RecordParams {

    public String filePath;

    public int width = 0;
    public int height = 0;
    public int frameRate;
    public int bitrate;

    public int bitsPerSample = 0;
    public int sampleRate = 0;
    public int numberOfChannels = 0;

    public RecordParams() {

    }

}
