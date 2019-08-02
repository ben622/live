#pragma once
//
// Created by ben622 on 2019/8/1.
//
#ifndef LIVE_CLASS_HPP
#define LIVE_CLASS_HPP

#include "signature_type.hpp"

namespace benlive {
    namespace jni {
        class Class {
        private:
            JNINativeMethod jniNativeMethod;


        public:
            jclass Clazz(JNIEnv *env) {
                return env->FindClass(Name());
            }

            const JNINativeMethod *
            METHOD(JNIEnv *env, const char *method, void *nativeMethodPeer) {
                const char *signature = benlive::jni::SIGNATURE(env, Name(), method);
                jniNativeMethod.name = method;
                jniNativeMethod.signature = signature;
                jniNativeMethod.fnPtr = nativeMethodPeer;
                return &jniNativeMethod;
            }

            virtual const char *Name() { return "com/ben/livesdk/NativePush";};
        };
    }
}
#endif //LIVE_CLASS_HPP
