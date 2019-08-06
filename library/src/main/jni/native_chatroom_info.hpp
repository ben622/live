//
// Created by ben622 on 2019/7/31.
//
#ifndef LIVE_NATIVE_CHATROOM_INFO_H
#define LIVE_NATIVE_CHATROOM_INFO_H
#include "include/jni/JniHelpers.h"
using namespace std;
using namespace benlive::jni;

namespace benlive {
    namespace entity {
        class NativeChatroomInfo {
        private:
            int id;
            int uid;
            int roomId;
            string roomName;
            string roomBgimg;
            string roomCover;
            string roomNotice;
            int peopleNum;
            int totalPeopleNum;
            int roomStatusCode;
            string nickName;
            string rtmpPushUrl;
            string rtmpPullUrl;
            string playBackUrl;
            string messageQueueId;
        public:

        };
    }
}

#endif //LIVE_NATIVE_CHATROOM_INFO_H
