//
// 使用libyuv对数据进行处理，旋转 缩放裁剪等
// Created by Administrator on 2019/9/5.
//

#ifndef LIVE_YUV_UTILS_H
#define LIVE_YUV_UTILS_H

#include "include/jni/JniHelpers.h"
#include "include/libyuv.h"

namespace benlive {
    namespace util {
        static void
        scaleI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data,
                  jint dst_width,
                  jint dst_height, jint mode) {

            jint src_i420_y_size = width * height;
            jint src_i420_u_size = (width >> 1) * (height >> 1);
            jbyte *src_i420_y_data = src_i420_data;
            jbyte *src_i420_u_data = src_i420_data + src_i420_y_size;
            jbyte *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

            jint dst_i420_y_size = dst_width * dst_height;
            jint dst_i420_u_size = (dst_width >> 1) * (dst_height >> 1);
            jbyte *dst_i420_y_data = dst_i420_data;
            jbyte *dst_i420_u_data = dst_i420_data + dst_i420_y_size;
            jbyte *dst_i420_v_data = dst_i420_data + dst_i420_y_size + dst_i420_u_size;

            libyuv::I420Scale((const uint8_t *) src_i420_y_data, width,
                              (const uint8_t *) src_i420_u_data, width >> 1,
                              (const uint8_t *) src_i420_v_data, width >> 1,
                              width, height,
                              (uint8_t *) dst_i420_y_data, dst_width,
                              (uint8_t *) dst_i420_u_data, dst_width >> 1,
                              (uint8_t *) dst_i420_v_data, dst_width >> 1,
                              dst_width, dst_height,
                              (libyuv::FilterMode) mode);
        }

        static void
        rotateI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data,
                   jint degree) {
            jint src_i420_y_size = width * height;
            jint src_i420_u_size = (width >> 1) * (height >> 1);

            jbyte *src_i420_y_data = src_i420_data;
            jbyte *src_i420_u_data = src_i420_data + src_i420_y_size;
            jbyte *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

            jbyte *dst_i420_y_data = dst_i420_data;
            jbyte *dst_i420_u_data = dst_i420_data + src_i420_y_size;
            jbyte *dst_i420_v_data = dst_i420_data + src_i420_y_size + src_i420_u_size;

            //要注意这里的width和height在旋转之后是相反的
            if (degree == libyuv::kRotate90 || degree == libyuv::kRotate270) {
                libyuv::I420Rotate((const uint8_t *) src_i420_y_data, width,
                                   (const uint8_t *) src_i420_u_data, width >> 1,
                                   (const uint8_t *) src_i420_v_data, width >> 1,
                                   (uint8_t *) dst_i420_y_data, height,
                                   (uint8_t *) dst_i420_u_data, height >> 1,
                                   (uint8_t *) dst_i420_v_data, height >> 1,
                                   width, height,
                                   (libyuv::RotationMode) degree);
            }
        }

        static void
        mirrorI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data) {
            jint src_i420_y_size = width * height;
            jint src_i420_u_size = (width >> 1) * (height >> 1);

            jbyte *src_i420_y_data = src_i420_data;
            jbyte *src_i420_u_data = src_i420_data + src_i420_y_size;
            jbyte *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

            jbyte *dst_i420_y_data = dst_i420_data;
            jbyte *dst_i420_u_data = dst_i420_data + src_i420_y_size;
            jbyte *dst_i420_v_data = dst_i420_data + src_i420_y_size + src_i420_u_size;

            libyuv::I420Mirror((const uint8_t *) src_i420_y_data, width,
                               (const uint8_t *) src_i420_u_data, width >> 1,
                               (const uint8_t *) src_i420_v_data, width >> 1,
                               (uint8_t *) dst_i420_y_data, width,
                               (uint8_t *) dst_i420_u_data, width >> 1,
                               (uint8_t *) dst_i420_v_data, width >> 1,
                               width, height);
        }


        static void
        nv21ToI420(jbyte *src_nv21_data, jint width, jint height, jbyte *src_i420_data) {
            jint src_y_size = width * height;
            jint src_u_size = (width >> 1) * (height >> 1);

            jbyte *src_nv21_y_data = src_nv21_data;
            jbyte *src_nv21_vu_data = src_nv21_data + src_y_size;

            jbyte *src_i420_y_data = src_i420_data;
            jbyte *src_i420_u_data = src_i420_data + src_y_size;
            jbyte *src_i420_v_data = src_i420_data + src_y_size + src_u_size;


            libyuv::NV21ToI420((const uint8_t *) src_nv21_y_data, width,
                               (const uint8_t *) src_nv21_vu_data, width,
                               (uint8_t *) src_i420_y_data, width,
                               (uint8_t *) src_i420_u_data, width >> 1,
                               (uint8_t *) src_i420_v_data, width >> 1,
                               width, height);
        }
    }
}
#endif //LIVE_YUV_UTILS_H
