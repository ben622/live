//
// Created by ben622 on 2019/7/31.
//
#include "jni.hpp"

namespace benlive {
    namespace android {
        JavaVM *theJVM;
        JNIEnv *jniEnv;
        //cache
        string cachePath;
        string dataPath;

        //注册相关native
        void registerNatives(JavaVM *vm) {
            theJVM = vm;
            vm->GetEnv((void **) &jniEnv, JNI_VERSION_1_6);

            //jni::Class<benlive::entity::NativeChatroomInfo>::Singleton(env);
        }
    }
}
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
    benlive::android::registerNatives(vm);
    return JNI_VERSION_1_6;
}