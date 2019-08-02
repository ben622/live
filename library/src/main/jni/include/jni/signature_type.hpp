//
// Created by ben622 on 2019/8/1.
//

#ifndef LIVE_SIGNATURE_TYPE_HPP
#define LIVE_SIGNATURE_TYPE_HPP

#include "jni.hpp"
#include "tagging.hpp"

#define PRINT_TAG "benlive"
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_VERBOSE,PRINT_TAG,FORMAT,__VA_ARGS__)
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,PRINT_TAG,FORMAT,__VA_ARGS__)
namespace benlive {
    namespace jni {
        static jclass signatureClass;
        static jmethodID signatureMethodID;
        //签名函数
        static jmethodID signatureMethod;

        static jobject
        getTargetClassMethodObject(JNIEnv *env, const char *javaClass, const char *method) {
            return env->CallStaticObjectMethod(signatureClass,
                                               signatureMethodID,
                                               env->NewStringUTF(javaClass),
                                               env->NewStringUTF(method));
        };

        static void registerNatives(JNIEnv *env) {
            signatureClass = env->FindClass(SignatureTag::Name());
            if (signatureClass == NULL) {
                LOGE("%s", "init signature types faild!");
                return;
            }

            signatureMethodID = env->GetStaticMethodID(signatureClass,
                                                       SignatureTag::getMethodByJavaClassNameInNativeMethod(),
                                                       SignatureTag::getMethodByJavaClassNameInNativeMethodSig());
            //获取签名函数ID
            signatureMethod = env->GetStaticMethodID(signatureClass,
                                                     SignatureTag::getSignatureMethod(),
                                                     SignatureTag::getSignatureMethodSig());
        };


        static const char *SIGNATURE(JNIEnv *env, const char *className, const char *method) {
            if (signatureClass == NULL || signatureMethod == NULL) {
                LOGE("%s", " Is this signature component registered?");
                return NULL;
            }
            jstring jsignature = static_cast<jstring>(env->CallStaticObjectMethod(signatureClass,
                                                                                  signatureMethod,
                                                                                  getTargetClassMethodObject(
                                                                                          env,
                                                                                          className,
                                                                                          method)
            ));
            const char *signature = env->GetStringUTFChars(jsignature, JNI_FALSE);
            LOGI("signature information:class[%s]-->method[%s]-->signature[%s]", className, method,
                 signature);
            return signature;
        }


    }

}
#endif //LIVE_SIGNATURE_TYPE_HPP
