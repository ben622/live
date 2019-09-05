//
// Created by ben622 on 2019/7/31.
//
#ifndef LIVE_NATIVE_PUSH_SERVICE_HPP
#define LIVE_NATIVE_PUSH_SERVICE_HPP

#include "include/jni/JniHelpers.h"
using namespace benlive::jni;
//c
extern "C" {
#include "include/queue.h"
#include "include/rtmp/rtmp.h"
};
namespace benlive {
    namespace service {
        /**
         * 流推送核心服务,维护推送队列
         * 1.视频推送
         * 2.音频推送
         * 3.消息推送
         *
         * 音视频使用RTMP协议推送<-->音视频队列
         * 消息进行队列入队 <--> 消息队列
         */
        class NativePushService {
        public:
            pthread_t mediaPushPthread;
            pthread_mutex_t mediaPushPthreadMutex;
            pthread_cond_t mediaPushPthreadCond;
            /////////////////////
            int startTime;
            int isPushing = false;

            NativePushService();
            void push(RTMPPacket *packet, bool updateTimeStamp);

            void stop();

            void destory();

            static NativePushService *getPushService();
        };
    }
}
static benlive::service::NativePushService *nativePushService;
#endif //LIVE_NATIVE_PUSH_SERVICE_HPP
