//
// Created by ben622 on 2019/7/31.
//
#ifndef LIVE_NATIVE_VIDEO_PUSH_HPP
#define LIVE_NATIVE_VIDEO_PUSH_HPP


#include "include/jni/JniHelpers.h"
#include "include/x264/x264.h"
#include "include/rtmp/rtmp.h"
#include "native_push_service.hpp"

#define SPS_OUT_BUFFER_SIZE 100
#define PPS_OUT_BUFFER_SIZE 100
using namespace benlive::jni;

//YUV长度
static int y_len, u_len, v_len;
static x264_param_t param;
static x264_picture_t pic;
static x264_picture_t pic_out;
static x264_t *x264_encoder;

namespace benlive {
    namespace push {
        class VideoPush : public JavaClass {
        public:
            VideoPush() : JavaClass() {}

            VideoPush(JNIEnv *env) : JavaClass(env) {
                initialize(env);
            }

            ~VideoPush() override {}

            void initialize(JNIEnv *env) override {
                setClass(env);

                addNativeMethod("setNativeVideoOptions", (void *) setNativeVideoOptions, kTypeVoid,
                                kTypeInt, kTypeInt, kTypeInt, kTypeInt,NULL);
                addNativeMethod("sendVideo", (void *) sendVideo, kTypeVoid, kTypeArray(kTypeByte),
                                NULL);
                addNativeMethod("prepare", (void *) prepare, kTypeVoid, NULL);
                addNativeMethod("startPush", (void *) startPush, kTypeVoid, NULL);
                addNativeMethod("pausePush", (void *) pausePush, kTypeVoid, NULL);
                addNativeMethod("stopPush", (void *) stopPush, kTypeVoid, NULL);
                addNativeMethod("free", (void *) nativeFree, kTypeVoid, NULL);

                registerNativeMethods(env);
            }

            const char *getCanonicalName() const override {
                return "com/ben/livesdk/NativePush";
            }

            void mapFields() override {}

            /**
            * 视频推送前初始化
            * @param env
            * @param width
            * @param height
            * @param bitrate
            * @param fps
            */
            static void
            setNativeVideoOptions(JNIEnv *env, jint width, jint height, jint bitrate, jint fps) {
                LOGI("%s", "setNativeVideoOptions...");
                LOGE(" width[%d], height[%d], bitrate[%d], fps[%d]", width,height,bitrate,fps);
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

            static void sendVideo(JNIEnv *env, jbyteArray jdata) {
                jbyte *data = env->GetByteArrayElements(jdata, NULL);
                //将NV21格式数据转换为YUV420
                //NV21转YUV420p的公式：(Y不变)Y=Y，U=Y+1+1，V=Y+1
                jbyte *y = reinterpret_cast<jbyte *>(pic.img.plane[0]);
                jbyte *u = reinterpret_cast<jbyte *>(pic.img.plane[1]);
                jbyte *v = reinterpret_cast<jbyte *>(pic.img.plane[2]);
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


                env->ReleaseByteArrayElements(jdata, data, 0);
            }



            /**
             * 添加视频序列消息头至rtmppacket中
             * @param sps
             * @param pps
             * @param sps_length
             * @param pps_length
             */
            static void
            add_squence_header_to_rtmppacket(unsigned char *sps, unsigned char *pps, int sps_length,
                                             int pps_length) {
                //packet内容大小
                int size = sps_length + pps_length + 16;
                RTMPPacket *packet = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
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


                //调用推送服务
                benlive::service::NativePushService::getPushService()->push(packet);
            }


            /**
             * 添加帧信息至rtmppacket
             * @param frame
             * @param length
             */
            static void add_frame_body_to_rtmppacket(unsigned char *frame, int len) {
                //去掉起始码四字节
                if (frame[2] == 0x00) {  //00 00 00 01
                    frame += 4;
                    len -= 4;
                } else if (frame[2] == 0x01) { // 00 00 01
                    frame += 3;
                    len -= 3;
                }
                RTMPPacket *packet = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
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

                benlive::service::NativePushService::getPushService()->push(packet);
            }



            static void prepare() {

            }

            static void startPush() {

            }

            static void pausePush() {

            }

            static void stopPush() {

            }

            static void nativeFree() {

            }

        };
    }
}

#endif //LIVE_NATIVE_VIDEO_PUSH_HPP