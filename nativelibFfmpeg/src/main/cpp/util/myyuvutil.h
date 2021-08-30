//
// Created by manjialle on 2021/8/27.
//

#ifndef MYFFMPEGLEARN_MYYUVUTIL_H
#define MYFFMPEGLEARN_MYYUVUTIL_H
#include "libyuv.h"

/**
 * 将nv21(yuv420sp)类型的数据转化成yuv210类型数据
 * @param src_nv21_data nv21数据
 * @param width 图像的宽
 * @param height 图像的高
 * @param src_i420_data 转化后的数据
 */
static inline void
nv21ToI420(uint8_t *src_nv21_data, int width, int height, uint8_t *dst_i420_data) {
    int src_y_size = width * height;
    int src_u_size = (width >> 1) * (height >> 1);

    uint8_t *src_nv21_y_data = src_nv21_data;
    uint8_t *src_nv21_vu_data = src_nv21_data + src_y_size;

    uint8_t *src_i420_y_data = dst_i420_data;
    uint8_t *src_i420_u_data = dst_i420_data + src_y_size;
    uint8_t *src_i420_v_data = dst_i420_data + src_y_size + src_u_size;

    libyuv::NV21ToI420(src_nv21_y_data, width,
                       src_nv21_vu_data, width,
                       src_i420_y_data, width,
                       src_i420_u_data, width >> 1,
                       src_i420_v_data, width >> 1,
                       width, height);

}

/**
 * 旋转yuv420数据.
 * @param src_i420_data yuv420原始数据
 * @param width 宽度
 * @param height 高度
 * @param dst_i420_data 转化后的数据
 * @param degree 旋转角度
 */
static inline void
rotateI420(uint8_t *src_i420_data, int width, int height, uint8_t *dst_i420_data, int degree) {
    int src_i420_y_size = width * height;
    int src_i420_u_size = (width >> 1) * (height >> 1);

    uint8_t *src_i420_y_data = src_i420_data;
    uint8_t *src_i420_u_data = src_i420_data + src_i420_y_size;
    uint8_t *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    uint8_t *dst_i420_y_data = dst_i420_data;
    uint8_t *dst_i420_u_data = dst_i420_data + src_i420_y_size;
    uint8_t *dst_i420_v_data = dst_i420_data + src_i420_y_size + src_i420_u_size;

    //要注意这里的width和height在旋转之后是相反的
    if (degree == libyuv::kRotate90 || degree == libyuv::kRotate270) {
        libyuv::I420Rotate(src_i420_y_data, width,
                           src_i420_u_data, width >> 1,
                           src_i420_v_data, width >> 1,
                           dst_i420_y_data, height,
                           dst_i420_u_data, height >> 1,
                           dst_i420_v_data, height >> 1,
                           width, height,
                           (libyuv::RotationMode) degree);
    }
}

static inline void n420_spin(uint8_t * dstyuv, uint8_t  *srcdata, int imageWidth, int imageHeight) {
    int i = 0, j = 0;
    int index = 0;
    int tempindex = 0;
    int div = 0;
    for (i = 0; i < imageWidth; i++) {
        div = i + 1;
        tempindex = 0;
        for (j = 0; j < imageHeight; j++) {
            tempindex += imageWidth;
            dstyuv[index++] = srcdata[tempindex - div];
        }
    }
    //写y 格式数据
   // fwrite(dstyuv, 1, (size_t) m_size, file_y);

    //u起始位置
    int start = imageWidth * imageHeight;
    //u v 数据的长度
    int udiv = start >> 2;
    //u v 数据宽度
    int uWidth = imageWidth >> 1;
    //u v 数据高度
    int uHeight = imageHeight >> 1;
    //数据 下标位置
    index = start;
    for (i = 0; i < uWidth; i++) {
        div = i + 1;
        tempindex = start;
        for (j = 0; j < uHeight; j++) {
            tempindex += uHeight;
            dstyuv[index] = srcdata[tempindex - div];
            dstyuv[index + udiv] = srcdata[tempindex - div + udiv];
            index++;
        }
    }
}

#endif //MYFFMPEGLEARN_MYYUVUTIL_H
