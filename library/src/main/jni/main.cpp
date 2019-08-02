//
// Created by ben622 on 2019/7/31.
//
#include "include/jni/jni.hpp"
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
    benlive::android::registerNatives(vm);
    return JNI_VERSION_1_6;
}