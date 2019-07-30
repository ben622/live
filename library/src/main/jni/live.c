//@author zhangchuan622@gmail.com
#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include "include/x264/x264.h"
#include "include/rtmp/rtmp.h"
#include "include/queue.h"
#include <pthread.h>
#include "include/faac/faac.h"

#define PRINT_TAG "bNativeLive"
#define PUSH_URL "rtmp://39.105.76.133:9510/live/benlive"

#define SPS_OUT_BUFFER_SIZE 100
#define PPS_OUT_BUFFER_SIZE 100
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_VERBOSE,PRINT_TAG,FORMAT,__VA_ARGS__)
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,PRINT_TAG,FORMAT,__VA_ARGS__)

x264_param_t param;
x264_picture_t pic;
x264_picture_t pic_out;
x264_t *x264_encoder;

pthread_t push_thread;
pthread_mutex_t push_thread_mutex;
pthread_cond_t push_cond;

//YUV长度
int y_len, u_len, v_len;
int start_time;

int is_pushing = FALSE;

//aac
unsigned long inputSamples;
unsigned long maxOutputBytes;
faacEncHandle faacEncodeHandle;

JNIEXPORT void JNICALL
Java_com_ben_livesdk_NativePush_setNativeVideoOptions(JNIEnv *env, jobject instance,
                                                           jint width, jint height, jint bitrate,
                                                           jint fps) {
    LOGI("%s", "setNativeVideoOptions...");

    //0延迟
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;

    //设置yuv长度
    y_len = width * height;
    u_len = y_len / 4;
    v_len = u_len;

    //码率控制，CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
    param.rc.i_rc_method = X264_RC_CRF;
    //码率 单位（Kbps）
    param.rc.i_bitrate = bitrate / 1000;
    //瞬时最大码率
    param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2;
    //通过fps控制码率，
    param.b_vfr_input = 0;
    //帧率分子
    param.i_fps_num = fps;
    //帧率分母
    param.i_fps_den = 1;
    param.i_timebase_den = param.i_fps_num;
    param.i_timebase_num = param.i_fps_den;
    //是否把SPS PPS放入每个关键帧，提高纠错能力
    param.b_repeat_headers = 1;
    //设置level级别，5.1
    param.i_level_idc = 51;

    //设置档次
    x264_param_apply_profile(&param, "baseline");

    x264_picture_alloc(&pic, param.i_csp, param.i_width, param.i_height);
    x264_encoder = x264_encoder_open(&param);
    if (x264_encoder) {
        LOGI("initVideoOptions:%s", "success");
    } else {
        LOGE("initVideoOptions:%s", "failed");
    }

}

/**
 * Faac初始化
 * Call faacEncOpen() for every encoder instance you need.
 *To set encoder options, call faacEncGetCurrentConfiguration(), change the parameters in the structure accessible by the returned pointer and then call faacEncSetConfiguration().
 *As long as there are still samples left to encode, call faacEncEncode() to encode the data. The encoder returns the bitstream data in a client-supplied buffer.
 *Once you call faacEncEncode() with zero samples of input the flushing process is initiated; afterwards you may call faacEncEncode() with zero samples input only.
 *faacEncEncode() will continue to write out data until all audio samples have been encoded.
 *Once faacEncEncode() has returned with zero bytes written, call faacEncClose() to destroy this encoder instance.
 * @param env
 * @param instance
 * @param sampleRateInHz
 * @param channel
 */
JNIEXPORT void JNICALL
Java_com_ben_livesdk_NativePush_setNativeAudioOptions(JNIEnv *env, jobject instance,
                                                           jint sampleRateInHz, jint channel) {
    faacEncodeHandle = faacEncOpen(sampleRateInHz, channel, &inputSamples, &maxOutputBytes);
    if (!faacEncodeHandle) {
        LOGE("%s", "FAAC encode open failed!");
        return;
    }
    faacEncConfigurationPtr faacEncodeConfigurationPtr = faacEncGetCurrentConfiguration(
            faacEncodeHandle);
    //指定MPEG版本
    faacEncodeConfigurationPtr->mpegVersion = MPEG4;
    faacEncodeConfigurationPtr->allowMidside = 1;
    faacEncodeConfigurationPtr->aacObjectType = LOW;
    faacEncodeConfigurationPtr->outputFormat = 0; //输出是否包含ADTS头
    faacEncodeConfigurationPtr->useTns = 1; //时域噪音控制,大概就是消爆音
    faacEncodeConfigurationPtr->useLfe = 0;
    faacEncodeConfigurationPtr->quantqual = 100;
    faacEncodeConfigurationPtr->bandWidth = 0; //频宽
    faacEncodeConfigurationPtr->shortctl = SHORTCTL_NORMAL;

    //call faacEncSetConfiguration
    if (!faacEncSetConfiguration(faacEncodeHandle, faacEncodeConfigurationPtr)) {
        LOGE("%s", "faacEncSetConfiguration failed！");
        return;
    }

    LOGI("%s", "faac initialization successful");
}


/**
 * 将packet添加至队列中
 * @param packet
 */
void add_rtmp_packet_queue(RTMPPacket *packet) {
    //lock
    pthread_mutex_lock(&push_thread_mutex);
    if (is_pushing) {
        queue_append_last(packet);
    }
    //signal
    pthread_cond_signal(&push_cond);
    //unlock
    pthread_mutex_unlock(&push_thread_mutex);
}


/**
 * 添加视频序列消息头至rtmppacket中
 * @param sps
 * @param pps
 * @param sps_length
 * @param pps_length
 */
void add_squence_header_to_rtmppacket(unsigned char *sps, unsigned char *pps, int sps_length,
                                      int pps_length) {
    //packet内容大小
    int size = sps_length + pps_length + 16;
    RTMPPacket *packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, size);
    RTMPPacket_Reset(packet);

    //设置packet中的body信息
    char *body = packet->m_body;
    int i = 0;
    /**
     * (1) FrameType，4bit，帧类型
            1 = key frame (for AVC, a seekable frame)
            2 = inter frame (for AVC, a non-seekable frame)
            3 = disposable inter frame (H.263 only)
            4 = generated key frame (reserved for server use only)
            5 = video info/command frame
            H264的一般为1或者2.
       (2)CodecID ，4bit，编码类型
            1 = JPEG(currently unused)
            2 = Sorenson H.263
            3 = Screen video
            4 = On2 VP6
            5 = On2 VP6 with alpha channel
            6 = Screen video version 2
            7 = AVC

     */
    //body 第一位
    body[i++] = 0x17; //(1)-(2)4bit*2关键帧，帧内压缩
    body[i++] = 0x00; //(3)8bit
    body[i++] = 0x00; //(4)8bit
    body[i++] = 0x00; //(5)8bit
    body[i++] = 0x00; //(6)8bit

    /*AVCDecoderConfigurationRecord*/
    body[i++] = 0x01;//configurationVersion，版本为1
    body[i++] = sps[1];//AVCProfileIndication
    body[i++] = sps[2];//profile_compatibility
    body[i++] = sps[3];//AVCLevelIndication

    body[i++] = 0xFF;//lengthSizeMinusOne,H264 视频中 NALU的长度，计算方法是 1 + (lengthSizeMinusOne & 3),实际测试时发现总为FF，计算结果为4.


    /*sps*/
    body[i++] = 0xE1;//numOfSequenceParameterSets:SPS的个数，计算方法是 numOfSequenceParameterSets & 0x1F,实际测试时发现总为E1，计算结果为1.
    body[i++] = (sps_length >> 8) & 0xff;//sequenceParameterSetLength:SPS的长度
    body[i++] = sps_length & 0xff;//sequenceParameterSetNALUnits
    memcpy(&body[i], sps, sps_length);
    i += sps_length;

    /*pps*/
    body[i++] = 0x01;//numOfPictureParameterSets:PPS 的个数,计算方法是 numOfPictureParameterSets & 0x1F,实际测试时发现总为E1，计算结果为1.
    body[i++] = (pps_length >> 8) & 0xff;//pictureParameterSetLength:PPS的长度
    body[i++] = (pps_length) & 0xff;//PPS
    memcpy(&body[i], pps, pps_length);
    i += pps_length;

    //设置packet头信息
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = size;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_nChannel = 0x04;//Audio和Video通道
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;


    add_rtmp_packet_queue(packet);

}


/**
 * 添加帧信息至rtmppacket
 * @param frame
 * @param length
 */
void add_frame_body_to_rtmppacket(unsigned char *frame, int len) {
    //去掉起始码四字节
    if (frame[2] == 0x00) {  //00 00 00 01
        frame += 4;
        len -= 4;
    } else if (frame[2] == 0x01) { // 00 00 01
        frame += 3;
        len -= 3;
    }
    RTMPPacket *packet = malloc(sizeof(RTMPPacket));
    int size = len + 9;
    RTMPPacket_Alloc(packet, size);
    RTMPPacket_Reset(packet);
    char *body = packet->m_body;
    /**
    * (1) FrameType，4bit，帧类型
           1 = key frame (for AVC, a seekable frame)
           2 = inter frame (for AVC, a non-seekable frame)
           3 = disposable inter frame (H.263 only)
           4 = generated key frame (reserved for server use only)
           5 = video info/command frame
           H264的一般为1或者2.
      (2)CodecID ，4bit，编码类型
           1 = JPEG(currently unused)
           2 = Sorenson H.263
           3 = Screen video
           4 = On2 VP6
           5 = On2 VP6 with alpha channel
           6 = Screen video version 2
           7 = AVC

    */
    //判断当前nalutype是关键帧I（帧内压缩）还是普通帧P（帧间压缩）
    //===================nal-type==========
    //5	IDR图像中的片 关键帧可以直接解压渲染
    //6	补充增强信息单元（SEI）
    //7	SPS（Sequence Parameter Set序列参数集，作用于一串连续的视频图像，即视频序列）
    //8	PPS（Picture Parameter Set图像参数集，作用于视频序列中的一个或多个图像
    //===================nal-type==========
    //nal组成
    //0 00 00000
    //禁止位 重要程度  nal-type
    //frame[0] = 5 = 00000101
    //00000101
    //&
    //00000111
    //00000101

    body[0] = 0x27;//非关键帧 帧间压缩
    int type = frame[0] & 0x1f;
    if (type == NAL_SLICE_IDR) {
        //关键帧
        body[0] = 0x17;
    }
    body[1] = 0x01; /*nal unit,NALUs（AVCPacketType == 1)*/
    body[2] = 0x00; //composition time 0x000000 24bit
    body[3] = 0x00;
    body[4] = 0x00;

    //写入NALU信息，右移8位，一个字节的读取？
    body[5] = (len >> 24) & 0xff;
    body[6] = (len >> 16) & 0xff;
    body[7] = (len >> 8) & 0xff;
    body[8] = (len) & 0xff;

    /*copy data*/
    memcpy(&body[9], frame, len);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = size;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;//当前packet的类型：Video
    packet->m_nChannel = 0x04; //Audio和Video通道
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nTimeStamp = RTMP_GetTime() - start_time;//记录了每一个tag相对于第一个tag（File Header）的相对时间
    add_rtmp_packet_queue(packet);
}

/**
 * @param env
 * @param instance
 * @param data_
 */
JNIEXPORT void JNICALL
Java_com_ben_livesdk_NativePush_sendVideo(JNIEnv *env, jobject instance, jbyteArray data_) {
    jbyte *data = (*env)->GetByteArrayElements(env, data_, NULL);
    //将NV21格式数据转换为YUV420
    //NV21转YUV420p的公式：(Y不变)Y=Y，U=Y+1+1，V=Y+1
    jbyte *y = pic.img.plane[0];
    jbyte *u = pic.img.plane[1];
    jbyte *v = pic.img.plane[2];
    //设置y
    memcpy(y, data, y_len);
    //设置u，v
    for (int i = 0; i < u_len; ++i) {
        *(u + i) = *(data + y_len + i * 2 + 1);
        *(v + i) = *(data + y_len + i * 2);
    }

    //使用x264编码
    x264_nal_t *nal = NULL;
    int n_nal = -1;
    if (x264_encoder_encode(x264_encoder, &nal, &n_nal, &pic, &pic_out) < 0) {
        LOGE("%s", "x264 encode error");
        return;
    }

    //设置SPS PPS
    unsigned char sps[SPS_OUT_BUFFER_SIZE];
    unsigned char pps[PPS_OUT_BUFFER_SIZE];
    int sps_length, pps_length;
    //reset
    memset(sps, 0, SPS_OUT_BUFFER_SIZE);
    memset(pps, 0, PPS_OUT_BUFFER_SIZE);

    pic.i_pts += 1; //顺序累加
    for (int i = 0; i < n_nal; ++i) {
        if (nal[i].i_type == NAL_SPS) {
            //00 00 00 01;07;payload
            //不复制四字节起始码，设置sps_length的长度为总长度-四字节起始码长度
            sps_length = nal[i].i_payload - 4;
            //复制sps数据
            memcpy(sps, nal[i].p_payload + 4, sps_length);
        } else if (nal[i].i_type == NAL_PPS) {
            pps_length = nal[i].i_payload - 4;
            memcpy(pps, nal[i].p_payload + 4, pps_length);

            //发送视频序列消息
            add_squence_header_to_rtmppacket(sps, pps, sps_length, pps_length);
        } else {
            //发送帧信息
            add_frame_body_to_rtmppacket(nal[i].p_payload, nal[i].i_payload);
        }

    }


    (*env)->ReleaseByteArrayElements(env, data_, data, 0);
}


/**
 * 将音频头信息发送
 * 音频头消息只发送一次
 */
void add_audio_squence_header_to_rtmppacket() {
    unsigned char *ppBuffer;
    unsigned long pSizeOfDecoderSpecificInfo;
    faacEncGetDecoderSpecificInfo(faacEncodeHandle, &ppBuffer, &pSizeOfDecoderSpecificInfo);
    //AAC Header占用2字节
    int size = pSizeOfDecoderSpecificInfo + 2;
    RTMPPacket *packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, size);
    RTMPPacket_Reset(packet);
    //设置packet中的body信息
    char *body = packet->m_body;

    /**
     * 1、SoundFormat，4bit
            0 = Linear PCM, platform endian
            1 = ADPCM
            2 = MP3
            3 = Linear PCM, little endian
            4 = Nellymoser 16 kHz mono
            5 = Nellymoser 8 kHz mono
            6 = Nellymoser
            7 = G.711 A-law logarithmic PCM
            8 = G.711 mu-law logarithmic PCM
            9 = reserved
            10 = AAC
            11 = Speex
            14 = MP3 8 kHz
            15 = Device-specific sound
       2、SoundRate，2bit，抽样频率
            0 = 5.5 kHz
            1 = 11 kHz
            2 = 22 kHz
            3 = 44 kHz
            对于AAC音频来说，总是0x11，即44khz.
        3、SoundSize，1bit，音频的位数。
            0 = 8-bit samples
            1 = 16-bit samples
            AAC总是为0x01,16位。
        4、SoundType，1bit，声道
            0 = Mono sound
            1 = Stereo sound
     */
    //10+3+1+1
    body[0] = 0xAF;
    body[1] = 0x00;
    //copy audio data
    memcpy(&body[2], ppBuffer, pSizeOfDecoderSpecificInfo);

    //设置rtmppacket
    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = size;
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nChannel = 0x04; //Audio和Video通道
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nTimeStamp = RTMP_GetTime() - start_time;//记录了每一个tag相对于第一个tag（File Header）的相对时间
    add_rtmp_packet_queue(packet);

}

/**
 * 使用rtmppacket将aac编码后的音频数据打包入队
 * @param bitbuf
 * @param byteslen
 */
void add_audio_body_to_rtmppacket(unsigned char *bitbuf, int byteslen) {
    //AAC Header占用2字节
    int size = byteslen + 2;
    RTMPPacket *packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, size);
    RTMPPacket_Reset(packet);
    //设置packet中的body信息
    char *body = packet->m_body;

    /**
     * 1、SoundFormat，4bit
            0 = Linear PCM, platform endian
            1 = ADPCM
            2 = MP3
            3 = Linear PCM, little endian
            4 = Nellymoser 16 kHz mono
            5 = Nellymoser 8 kHz mono
            6 = Nellymoser
            7 = G.711 A-law logarithmic PCM
            8 = G.711 mu-law logarithmic PCM
            9 = reserved
            10 = AAC
            11 = Speex
            14 = MP3 8 kHz
            15 = Device-specific sound
       2、SoundRate，2bit，抽样频率
            0 = 5.5 kHz
            1 = 11 kHz
            2 = 22 kHz
            3 = 44 kHz
            对于AAC音频来说，总是0x11，即44khz.
        3、SoundSize，1bit，音频的位数。
            0 = 8-bit samples
            1 = 16-bit samples
            AAC总是为0x01,16位。
        4、SoundType，1bit，声道
            0 = Mono sound
            1 = Stereo sound
        5、AACPacketType，8bit。
这个字段来表示AACAUDIODATA的类型：0 = AAC sequence header，1 = AAC raw。第一个音频包用0，后面的都用1。
     */
    //10+3+1+1
    body[0] = 0xAF;
    body[1] = 0x01;
    //copy audio data
    memcpy(&body[2], bitbuf, byteslen);

    //设置rtmppacket
    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = size;
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nChannel = 0x04; //Audio和Video通道
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nTimeStamp = RTMP_GetTime() - start_time;//记录了每一个tag相对于第一个tag（File Header）的相对时间
    add_rtmp_packet_queue(packet);
}

/**
 * 使用AAC进行音频编码
 * @param env
 * @param instance
 * @param audioData_
 * @param offsetInBytes
 * @param sizeInBytes
 */
JNIEXPORT void JNICALL
Java_com_ben_livesdk_NativePush_sendAudio(JNIEnv *env, jobject instance, jbyteArray audioData_,
                                               jint offsetInBytes, jint sizeInBytes) {
    jbyte *audioData = (*env)->GetByteArrayElements(env, audioData_, NULL);
    int *pcmbuf;
    unsigned char *bitbuf;
    pcmbuf = (short *) malloc(inputSamples * sizeof(int));
    bitbuf = (unsigned char *) malloc(maxOutputBytes * sizeof(unsigned char));
    int nByteCount = 0;
    unsigned int nBufferSize = (unsigned int) sizeInBytes / 2;
    unsigned short *buf = (unsigned short *) audioData;
    while (nByteCount < nBufferSize) {
        int audioLength = inputSamples;
        if ((nByteCount + inputSamples) >= nBufferSize) {
            audioLength = nBufferSize - nByteCount;
        }
        int i;
        for (i = 0; i < audioLength; i++) {//每次从实时的pcm音频队列中读出量化位数为8的pcm数据。
            int s = ((int16_t *) buf + nByteCount)[i];
            pcmbuf[i] = s << 8;//用8个二进制位来表示一个采样量化点（模数转换）
        }
        nByteCount += inputSamples;
        //利用FAAC进行编码，pcmbuf为转换后的pcm流数据，audioLength为调用faacEncOpen时得到的输入采样数，bitbuf为编码后的数据buff，nMaxOutputBytes为调用faacEncOpen时得到的最大输出字节数
        int byteslen = faacEncEncode(faacEncodeHandle, pcmbuf, audioLength,
                                     bitbuf, maxOutputBytes);
        if (byteslen < 1) {
            continue;
        }
        add_audio_body_to_rtmppacket(bitbuf, byteslen);//从bitbuf中得到编码后的aac数据流，放到数据队列
    }
    (*env)->ReleaseByteArrayElements(env, audioData_, audioData, 0);
    if (bitbuf)
        free(bitbuf);
    if (pcmbuf)
        free(pcmbuf);


}

JNIEXPORT void JNICALL
Java_com_ben_livesdk_NativePush_prepare(JNIEnv *env, jobject instance) {

    // TODO

}

JNIEXPORT void JNICALL
Java_com_ben_livesdk_NativePush_stopPush(JNIEnv *env, jobject instance) {
    is_pushing = FALSE;
    // TODO

}

JNIEXPORT void JNICALL
Java_com_ben_livesdk_NativePush_free(JNIEnv *env, jobject instance) {

    // TODO

}

/**
 * 从队列中读取Packet，使用RTMP发送
 * @param arg
 * @return
 */
void *push_thread_func(void *arg) {
    RTMP *rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    //设置连接超时时间
    rtmp->Link.timeout = 10;
    RTMP_SetupURL(rtmp, PUSH_URL);
    //发送rtmp数据
    RTMP_EnableWrite(rtmp);
    //建立连接
    if (!RTMP_Connect(rtmp, NULL)) {
        LOGE("connect [%s] result:%s", PUSH_URL, "建立连接失败！");
        goto end;
    } else {
        LOGE("connect [%s] result:%s", PUSH_URL, "successful");
    }
    start_time = RTMP_GetTime();
    if (!RTMP_ConnectStream(rtmp, 0)) {
        LOGE("connect [%s] result:%s", PUSH_URL, "RTMP_ConnectStream failed!");
        goto end;
    }
    is_pushing = TRUE;
    //send audio header packet
    add_audio_squence_header_to_rtmppacket();
    //send
    while (is_pushing) {
        pthread_mutex_lock(&push_thread_mutex);
        pthread_cond_wait(&push_cond, &push_thread_mutex);
        //从队列中获取第一个packet
        RTMPPacket *packet = queue_get_first();
        if (packet) {
            //移除
            queue_delete_first();
            packet->m_nInfoField2 = rtmp->m_stream_id; //RTMP协议，stream_id数据
            int i = RTMP_SendPacket(rtmp, packet, TRUE); //TRUE放入librtmp队列中，并不是立即发送
            if (!i) {
                RTMPPacket_Free(packet);
                pthread_mutex_unlock(&push_thread_mutex);
                goto end;
            } else {
                LOGI("%s", "rtmp send packet");
            }
            RTMPPacket_Free(packet);
        }
        pthread_mutex_unlock(&push_thread_mutex);
    }


    end:
    RTMP_Close(rtmp);
    RTMP_Free(rtmp);
    return 0;
}
/**
 * 当开始直播时
 * @param env
 * @param instance
 */
JNIEXPORT void JNICALL
Java_com_ben_livesdk_NativePush_startPush(JNIEnv *env, jobject instance) {
    //创建队列
    create_queue();
    //初始化线程
    pthread_mutex_init(&push_thread_mutex, NULL);
    pthread_cond_init(&push_cond, NULL);
    pthread_create(&push_thread, NULL, push_thread_func, NULL);

}

JNIEXPORT void JNICALL
Java_com_ben_livesdk_NativePush_pausePush(JNIEnv *env, jobject instance) {

    // TODO

}