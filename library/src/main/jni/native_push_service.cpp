//
// Created by ben622 on 2019/7/31.
//

#include "native_push_service.hpp"

#define PUSH_URL "rtmp://39.105.76.133:9510/live/benlive"

using namespace benlive::service;

//pthread callback
void *pthreadCallback(void *arg) {
    NativePushService *nativePushService = static_cast<NativePushService *>(arg);

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
        LOGI("connect [%s] result:%s", PUSH_URL, "successful");
    }
    nativePushService->startTime = RTMP_GetTime();
    if (!RTMP_ConnectStream(rtmp, 0)) {
        LOGE("connect [%s] result:%s", PUSH_URL, "RTMP_ConnectStream failed!");
        goto end;
    }
    nativePushService->isPushing = TRUE;
    //send audio header packet
    //add_audio_squence_header_to_rtmppacket();
    //send
    while (nativePushService->isPushing) {
        pthread_mutex_lock(&nativePushService->mediaPushPthreadMutex);
        pthread_cond_wait(&nativePushService->mediaPushPthreadCond, &nativePushService->mediaPushPthreadMutex);
        //从队列中获取第一个packet
        RTMPPacket *packet = static_cast<RTMPPacket *>(queue_get_first());
        if (packet) {
            //移除
            queue_delete_first();
            packet->m_nInfoField2 = rtmp->m_stream_id; //RTMP协议，stream_id数据
            int i = RTMP_SendPacket(rtmp, packet, TRUE); //TRUE放入librtmp队列中，并不是立即发送
            if (!i) {
                RTMPPacket_Free(packet);
                pthread_mutex_unlock(&nativePushService->mediaPushPthreadMutex);
                goto end;
            } else {
                //LOGI("%s", "rtmp send packet");
            }
            RTMPPacket_Free(packet);
        }
        pthread_mutex_unlock(&nativePushService->mediaPushPthreadMutex);
    }

    end:
    RTMP_Close(rtmp);
    RTMP_Free(rtmp);

    return 0;
}

NativePushService::NativePushService() {
    //创建队列
    create_queue();
    //初始化thread
    pthread_mutex_init(&mediaPushPthreadMutex, NULL);
    pthread_cond_init(&mediaPushPthreadCond, NULL);
    pthread_create(&mediaPushPthread, NULL, pthreadCallback, this);
}

NativePushService *NativePushService::getPushService() {
    if (nativePushService == NULL) {
        nativePushService = new NativePushService();
    }
    return nativePushService;
}


//将packet加入到推送队列中,音视频数据单元都必须装载至packet进行推送
void NativePushService::push(RTMPPacket *packet) {
    //记录了每一个tag相对于第一个tag（File Header）的相对时间
    packet->m_nTimeStamp = RTMP_GetTime() - startTime;
    //lock
    pthread_mutex_lock(&mediaPushPthreadMutex);
    if (isPushing) {
        queue_append_last(packet);
    }
    //signal
    pthread_cond_signal(&mediaPushPthreadCond);
    //unlock
    pthread_mutex_unlock(&mediaPushPthreadMutex);

}

//停止服务释放资源，一般在直播结束时被调用
void NativePushService::destory() {
    //释放线程
    pthread_mutex_destroy(&mediaPushPthreadMutex);
    pthread_cond_destroy(&mediaPushPthreadCond);
    pthread_detach(mediaPushPthread);
}
void NativePushService::stop() {
    isPushing = false;
}


