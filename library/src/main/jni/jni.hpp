//
// Created by ben622 on 2019/7/31.
//
#ifndef JNI_HEAD
#define JNI_HEAD

#include <cstdlib>
#include <stdlib.h>
#include <jni.h>
#include <android/log.h>
#include <string>
#include <pthread.h>

#define PRINT_TAG "benlive"
#define EMAIL "zhuanchuan622@gmail.com"
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_VERBOSE,PRINT_TAG,FORMAT,__VA_ARGS__)
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,PRINT_TAG,FORMAT,__VA_ARGS__)
using namespace std;
#endif

