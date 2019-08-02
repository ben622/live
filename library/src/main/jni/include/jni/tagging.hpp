//
// Created by ben622 on 2019/8/1.
//

#ifndef LIVE_TAGGING_HPP
#define LIVE_TAGGING_HPP

struct ClassTag {
    static const char *Name() {
        return "java/lang/Class";
    }

    static const char *getDeclaredMethod() {
        return "getDeclaredMethod";
    }

    static const char *getDeclaredMethodSig() {
        return "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;";
    }
};

struct StringTag {
    static const char *Name() {
        return "java/lang/String";
    }
};

struct SignatureTag {
    static const char *Name() {
        return "com/ben/livesdk/utils/SignatureTypes";
    }

    static const char *Constructor() {
        return "<init>";
    }

    static const char *ConstructorSig() {
        return "()V";
    }

    static const char *getSignatureMethod() {
        return "getSignature";
    }

    static const char *getSignatureMethodSig() {
        return "(Ljava/lang/reflect/Method;)Ljava/lang/String;";
    }


    static const char *getMethodByJavaClassNameInNativeMethod() {
        return "getMethodByJavaClassNameInNative";
    }

    static const char *getMethodByJavaClassNameInNativeMethodSig() {
        return "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/reflect/Method;";
    }
};

#endif //LIVE_TAGGING_HPP
