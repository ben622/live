//
// Created by ben622 on 2019/9/7.
// openvc滤镜
//
#ifndef LIVE_NATIVE_FILTER_OPENVC_IMPL_H
#define LIVE_NATIVE_FILTER_OPENVC_IMPL_H

#include "include/jni/JniHelpers.h"
#include <math.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/imgproc/types_c.h>
#include "include/opencv2/core.hpp"
#include "include/opencv2/imgproc.hpp"

using namespace cv;
using namespace std;
extern "C" {
namespace benlive {
    namespace filter {
        /**
         * 毛玻璃滤镜
         * @param width
         * @param height
         * @param src
         */
        static void diffusionFilter(int width, int height, cv::Mat *src) {
            cv::Mat pNewMat;
            RNG rng;
            pNewMat = pNewMat.zeros(src->size(), src->type());
            for (int y = 1; y < height - 1; y++) {
                uchar *P0 = src->ptr<uchar>(y);
                uchar *P1 = pNewMat.ptr<uchar>(y);
                for (int x = 1; x < width - 1; x++) {
                    int tmp = rng.uniform(0, 9);
                    P1[3 * x] = src->at<uchar>(y - 1 + tmp / 3, 3 * (x - 1 + tmp % 3));
                    P1[3 * x + 1] = src->at<uchar>(y - 1 + tmp / 3, 3 * (x - 1 + tmp % 3) + 1);
                    P1[3 * x + 2] = src->at<uchar>(y - 1 + tmp / 3, 3 * (x - 1 + tmp % 3) + 2);
                }
            }

            pNewMat.copyTo(*src);
        }

        /**
         * 素描滤镜
         * @param width
         * @param height
         * @param src
         */
        static void sketchFilter(int width, int height, Mat *src) {
            Mat gray0,gray1;
            //去色
            //cvtColor(src, gray0, CV_BGR2GRAY);
            //反色
            addWeighted(gray0,-1,NULL,0,255,gray1);
            //高斯模糊,高斯核的Size与最后的效果有关
            GaussianBlur(gray1,gray1,Size(11,11),0);

            //融合：颜色减淡
            Mat img(gray1.size(),CV_8UC1);
            for (int y=0; y<height; y++)
            {

                uchar* P0  = gray0.ptr<uchar>(y);
                uchar* P1  = gray1.ptr<uchar>(y);
                uchar* P  = img.ptr<uchar>(y);
                for (int x=0; x<width; x++)
                {
                    int tmp0=P0[x];
                    int tmp1=P1[x];
                    P[x] =(uchar) min((tmp0+(tmp0*tmp1)/(256-tmp1)),255);
                }

            }
            img.copyTo(*src);
        }


        static void opencvDiffusionFilter(int width, int height, jbyte *yuv, jbyte *dst) {
            int size = width * height;
            Mat srcyuv(height, width, CV_8UC1, yuv);
            Mat detrgb(height, width, CV_8UC1);
            cvtColor(srcyuv, detrgb, CV_YUV2BGR_YV12);
            //diffusionFilter(width, height, &detrgb);
            //sketchFilter(width, height, detrgb);

            Mat gray0,gray1;
            //去色
            cvtColor(detrgb, gray0, CV_BGR2GRAY);
            //反色
            addWeighted(gray0,-1,NULL,0,255,gray1);
            //高斯模糊,高斯核的Size与最后的效果有关
            GaussianBlur(gray1,gray1,Size(11,11),0);

            //融合：颜色减淡
            Mat img(gray1.size(),CV_8UC1);
            for (int y=0; y<height; y++)
            {

                uchar* P0  = gray0.ptr<uchar>(y);
                uchar* P1  = gray1.ptr<uchar>(y);
                uchar* P  = img.ptr<uchar>(y);
                for (int x=0; x<width; x++)
                {
                    int tmp0=P0[x];
                    int tmp1=P1[x];
                    P[x] =(uchar) min((tmp0+(tmp0*tmp1)/(256-tmp1)),255);
                }

            }
            img.copyTo(detrgb);


            cvtColor(detrgb, srcyuv, CV_BGR2YUV_YV12);

            memcpy(dst, srcyuv.data,
                   width * height * 3 / 2 );
        }


    }
}
}
#endif //LIVE_NATIVE_FILTER_OPENVC_IMPL_H
