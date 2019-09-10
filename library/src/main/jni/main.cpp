//
// Created by ben622 on 2019/7/31.
//
#ifndef LIVE_MAIN_CPP
#define LIVE_MAIN_CPP

#include "include/jni/JniHelpers.h"
#include "native_video_push.cpp"
#include "native_audio_push.cpp"


ClassRegistry registry;;

using namespace benlive::jni;

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *jvm, void *) {
    JNIEnv *env = jniHelpersInitialize(jvm);
    if (env == NULL) {
        return -1;
    }
    //register
    registry.add(env, new benlive::push::VideoPush(env));
    registry.add(env, new benlive::push::AudioPush(env));
    return JNI_VERSION_1_6;
}
#endif //LIVE_MAIN_CPP