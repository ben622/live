//
// Created by ben622 on 2019/7/31.
//

#include "native_push_service.hpp"

#define PUSH_URL "rtmp://39.105.76.133:9510/live/benlive"

namespace benlive {
    namespace service {
        //pthread callback
        void *pthreadCallback(void *arg) {
            return 0;
        }

        //初始化推送服务
        void NativePushService::init() {
            //创建队列
            create_queue();
            //初始化thread
            pthread_mutex_init(&mediaPushPthreadMutex, NULL);
            pthread_cond_init(&mediaPushPthreadCond, NULL);
            pthread_create(&mediaPushPthread, NULL, pthreadCallback, NULL);
        }

        //将packet加入到推送队列中,音视频数据单元都必须装载至packet进行推送
        void NativePushService::push(RTMPPacket *packet) {
            RTMP *rtmp = RTMP_Alloc();
            RTMP_Init(rtmp);
            //设置连接超时时间
            rtmp->Link.timeout = 10;
            RTMP_SetupURL(rtmp, PUSH_URL);
            //发送rtmp数据
            RTMP_EnableWrite(rtmp);
            //建立连接
            if (!RTMP_Connect(rtmp, NULL)) {
                LOGE("connect [%s] result:%s", PUSH_URL, "连接服务器失败！");
                goto end;
            } else {
                LOGE("connect [%s] result:%s", PUSH_URL, "successful");
            }
            startTime = RTMP_GetTime();
            if (!RTMP_ConnectStream(rtmp, 0)) {
                LOGE("connect [%s] result:%s", PUSH_URL, "RTMP_ConnectStream failed!");
                goto end;
            }
            isPushing = TRUE;
            //send audio header packet
            //add_audio_squence_header_to_rtmppacket();
            //send
            while (isPushing) {
                pthread_mutex_lock(&mediaPushPthreadMutex);
                pthread_cond_wait(&mediaPushPthreadCond, &mediaPushPthreadMutex);
                //从队列中获取第一个packet
                RTMPPacket *packet = static_cast<RTMPPacket*>(queue_get_first());
                if (packet) {
                    //移除
                    queue_delete_first();
                    packet->m_nInfoField2 = rtmp->m_stream_id; //RTMP协议，stream_id数据
                    int i = RTMP_SendPacket(rtmp, packet, TRUE); //TRUE放入librtmp队列中，并不是立即发送
                    if (!i) {
                        RTMPPacket_Free(packet);
                        pthread_mutex_unlock(&mediaPushPthreadMutex);
                        goto end;
                    } else {
                        LOGI("%s", "rtmp send packet");
                    }
                    RTMPPacket_Free(packet);
                }
                pthread_mutex_unlock(&mediaPushPthreadMutex);
            }

            end:
            RTMP_Close(rtmp);
            RTMP_Free(rtmp);
        }

        //停止服务释放资源，一般在直播结束时被调用
        void NativePushService::destory() {
            //释放线程
            pthread_mutex_destroy(&mediaPushPthreadMutex);
            pthread_cond_destroy(&mediaPushPthreadCond);
            pthread_detach(mediaPushPthread);
        }


    }
}
