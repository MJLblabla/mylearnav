package com.cxp.nativelibffmpeg.utils;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.widget.ImageView;

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;

public class YUVJavaUtil {

    public static void nv21ToI420(byte[] ret,byte[] data, int width, int height) {

        int total = width * height;

        ByteBuffer bufferY = ByteBuffer.wrap(ret, 0, total);
        ByteBuffer bufferU = ByteBuffer.wrap(ret, total, total / 4);
        ByteBuffer bufferV = ByteBuffer.wrap(ret, total + total / 4, total / 4);

        bufferY.put(data, 0, total);
        for (int i=total; i<data.length; i+=2) {
            bufferV.put(data[i]);
            bufferU.put(data[i+1]);
        }
    }
    public static byte[] I420ToNV21(byte[] input, int width, int height) {
        byte[] output = new byte[input.length];
        int frameSize = width * height;
        int qFrameSize = frameSize / 4;
        int tempFrameSize = frameSize * 5 / 4;
        System.arraycopy(input, 0, output, 0, frameSize);

        for(int i = 0; i < qFrameSize; ++i) {
            output[frameSize + i * 2] = input[tempFrameSize + i];
            output[frameSize + i * 2 + 1] = input[frameSize + i];
        }

        return output;
    }

    //I420 转 NV12数据
    public static void I420ToNV12(byte[] I420, byte[] NV12, int width, int height){
        int ySize = width * height;
        int yuvSize = width * height * 3 / 2;
        int uIndex = ySize;
        int vIndex = ySize * 5 / 4;
        System.arraycopy(I420, 0, NV12, 0, ySize);
        for (int i = ySize; i < yuvSize; i += 2){
            NV12[i] = I420[uIndex++];
            NV12[i+1] = I420[vIndex++];
        }
    }

    public static void rgbEncodeYUV420SP(byte[] nv21, int[] argb, int width, int height) {
        int frameSize = width * height;
        int yIndex = 0;
        int uvIndex = frameSize;
        int index = 0;

        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < width; ++i) {
                int R = (argb[index] & 0xFF0000) >> 16;
                int G = (argb[index] & 0x00FF00) >> 8;
                int B = argb[index] & 0x0000FF;
                int Y = (66 * R + 129 * G + 25 * B + 128 >> 8) + 16;
                int U = (-38 * R - 74 * G + 112 * B + 128 >> 8) + 128;
                int V = (112 * R - 94 * G - 18 * B + 128 >> 8) + 128;
                nv21[yIndex++] = (byte) (Y < 0 ? 0 : (Y > 255 ? 255 : Y));
                if (j % 2 == 0 && index % 2 == 0 && uvIndex < nv21.length - 2) {
                    nv21[uvIndex++] = (byte) (V < 0 ? 0 : (V > 255 ? 255 : V));
                    nv21[uvIndex++] = (byte) (U < 0 ? 0 : (U > 255 ? 255 : U));
                }

                ++index;
            }
        }
    }

    public static void test(final ImageView iv , byte[] bt, int width, int height){
        if(iv==null){
            return;
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        YuvImage yuvImage = new YuvImage(bt, ImageFormat.NV21, width, height, null);

        yuvImage.compressToJpeg(new Rect(0, 0, width, height), 50, out);
        byte[] imageBytes = out.toByteArray();
        final Bitmap image = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.length);
        iv.post(new Runnable() {
            @Override
            public void run() {
                iv.setImageBitmap(image);
            }
        });
    }

}
