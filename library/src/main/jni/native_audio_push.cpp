//
// Created by ben622 on 2019/7/31.
//
#ifndef LIVE_NATIVE_AUDIO_PUSH_HPP
#define LIVE_NATIVE_AUDIO_PUSH_HPP

#include "include/jni/JniHelpers.h"
#include "include/faac/faac.h"
#include "include/rtmp/rtmp.h"
#include "native_push_service.hpp"
using namespace benlive::jni;

static unsigned long inputSamples;
static unsigned long maxOutputBytes;
static faacEncHandle faacEncodeHandle;

namespace benlive {
    namespace push {
        class AudioPush : public JavaClass {
        public:
            AudioPush() : JavaClass() {}

            AudioPush(JNIEnv *env) : JavaClass(env) {
                initialize(env);
            }

            ~AudioPush() override {}

            void initialize(JNIEnv *env) override {
                setClass(env);
                addNativeMethod("setNativeAudioOptions", (void *) setNativeAudioOptions, kTypeVoid,
                                kTypeInt, kTypeInt, NULL);
                addNativeMethod("sendAudio", (void *) sendAudio, kTypeVoid, kTypeArray(kTypeByte),
                                kTypeInt, kTypeInt,
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
             * Faac初始化
             * Call faacEncOpen() for every encoder instance you need.
             * To set encoder options, call faacEncGetCurrentConfiguration(), change the parameters in the structure accessible by the returned pointer and then call faacEncSetConfiguration().
             * As long as there are still samples left to encode, call faacEncEncode() to encode the data. The encoder returns the bitstream data in a client-supplied buffer.
             * Once you call faacEncEncode() with zero samples of input the flushing process is initiated; afterwards you may call faacEncEncode() with zero samples input only.
             * faacEncEncode() will continue to write out data until all audio samples have been encoded.
             * Once faacEncEncode() has returned with zero bytes written, call faacEncClose() to destroy this encoder instance.
             * @param env
             * @param sampleRateInHz
             * @param channel
             */
            static void setNativeAudioOptions(JNIEnv *env, jint sampleRateInHz, jint channel) {
                LOGE("sampleRateInHz[%d],channel[%d]", sampleRateInHz, channel);
                faacEncodeHandle = faacEncOpen(sampleRateInHz, channel, &inputSamples,
                                               &maxOutputBytes);
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
             * 使用rtmppacket将aac编码后的音频数据打包入队
             * @param bitbuf
             * @param byteslen
             */
            static void add_audio_body_to_rtmppacket(unsigned char *bitbuf, int byteslen) {
                //AAC Header占用2字节
                int size = byteslen + 2;
                RTMPPacket *packet = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
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

                benlive::service::NativePushService::getPushService()->push(packet);
            }

            static void
            sendAudio(JNIEnv *env, jbyteArray jdata, int offsetInBytes, int sizeInBytes) {
                jbyte *audioData = env->GetByteArrayElements(jdata, NULL);
                int *pcmbuf;
                unsigned char *bitbuf;
                pcmbuf = (int *) malloc(inputSamples * sizeof(int));
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
                env->ReleaseByteArrayElements(jdata, audioData, 0);
                if (bitbuf)
                    free(bitbuf);
                if (pcmbuf)
                    free(pcmbuf);
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
#endif //LIVE_NATIVE_AUDIO_PUSH_HPP

