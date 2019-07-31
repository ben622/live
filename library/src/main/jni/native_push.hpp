//
// Created by ben622 on 2019/7/31.
//

#ifndef LIVE_NATIVE_PUSH_HPP
#define LIVE_NATIVE_PUSH_HPP

#include "jni.hpp"

//benlive::push
namespace benlive {
    namespace push {
        class Push {
        public:
            virtual void prepare();

            virtual void startPush();

            virtual void pausePush();

            virtual void stopPush();

            virtual void free();
        };
    }
}
#endif //LIVE_NATIVE_PUSH_HPP
