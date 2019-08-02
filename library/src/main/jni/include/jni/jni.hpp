#pragma once
//
// Created by ben622 on 2019/7/31.
//
#ifndef JNI_HEAD
#define JNI_HEAD

#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <jni.h>
#include <android/log.h>
#include <string>
#include <pthread.h>
#include "class.hpp"
#include "../../native_push.hpp"
#include "signature_type.hpp"

#define PRINT_TAG "benlive"
#define EMAIL "zhuanchuan622@gmail.com"
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_VERBOSE,PRINT_TAG,FORMAT,__VA_ARGS__)
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,PRINT_TAG,FORMAT,__VA_ARGS__)

using namespace std;

namespace benlive {
    namespace jni {
        //onload init set.
        static JavaVM *theJVM;
        static JNIEnv *jniEnv;

        static void RegisterNativePeer(JNIEnv *env, Class clazz, const char *fieldName,
                                       void *nativeMethodPeer) {
            const JNINativeMethod *method = clazz.METHOD(jniEnv, fieldName, nativeMethodPeer);
            jniEnv->RegisterNatives(clazz.Clazz(jniEnv), method, 1);

        }

        /*//onload init register jni.
        static void RegisterNatives() {
        }*/
    }
}

//onload
namespace benlive {
    namespace android {
        static void test() {
            LOGE("%s", "test");
        }

        //registers
        inline void registerNatives(JavaVM *vm) {
            benlive::jni::theJVM = vm;
            vm->GetEnv((void **) &benlive::jni::jniEnv, JNI_VERSION_1_6);
            benlive::jni::registerNatives(benlive::jni::jniEnv);
            //init register jni
            benlive::jni::RegisterNativePeer(benlive::jni::jniEnv,
                                             benlive::jni::Class(), "test",
                                             (void *) test);
        }
    }
}

#endif //JNI_HEAD

