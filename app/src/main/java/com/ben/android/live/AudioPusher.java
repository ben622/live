package com.ben.android.live;

import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

public class AudioPusher extends APusher {
    private Builder builder;
    private int bufferSize;
    private AudioRecord audioRecord;
    private Thread mAudioThread;

    public AudioPusher(Builder builder) {
        this.builder = builder;
    }

    @Override
    public void prepare() {
        bufferSize = AudioRecord.getMinBufferSize(builder.getSampleRateInHz(), builder.getChannelConfig(), builder.getAudioFormat());
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, builder.getSampleRateInHz(), builder.getChannelConfig(), builder.getAudioFormat(), bufferSize);

    }

    @Override
    public void startPush() {
        isPushing = true;
        builder.getNativePush().setNativeAudioOptions(builder.getSampleRateInHz(), builder.getChannelConfig());
        mAudioThread = new Thread(new AudioPushTask());
        mAudioThread.start();
    }

    @Override
    public void pausePush() {
        isPushing = false;
    }

    @Override
    public void stopPush() {
        isPushing = false;
        builder.getNativePush().stopPush();
    }

    @Override
    public void free() {
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }
        builder.getNativePush().free();
    }


    public static class Builder {
        private int audioSource;
        private int sampleRateInHz;
        private int channelConfig;
        private int audioFormat;
        private int bufferSizeInByte;

        private NativePush nativePush;


        public int getAudioSource() {
            return audioSource;
        }


        public int getSampleRateInHz() {
            return sampleRateInHz;
        }

        public Builder sampleRateInHz(int sampleRateInHz) {
            this.sampleRateInHz = sampleRateInHz;
            return this;
        }

        public int getChannelConfig() {
            return channelConfig;
        }

        public Builder channelConfig(int channelConfig) {
            this.channelConfig = channelConfig;
            return this;
        }

        public int getAudioFormat() {
            return audioFormat;
        }

        public Builder audioFormat(int audioFormat) {
            this.audioFormat = audioFormat;
            return this;
        }

        public int getBufferSizeInByte() {
            return bufferSizeInByte;
        }

        public NativePush getNativePush() {
            return nativePush;
        }

        public Builder nativePush(NativePush nativePush) {
            this.nativePush = nativePush;
            return this;
        }

        public AudioPusher build() {
            return new AudioPusher(this);
        }
    }

    private class AudioPushTask implements Runnable {

        @Override
        public void run() {
            audioRecord.startRecording();

            while (isPushing && audioRecord != null) {
                byte[] buffer = new byte[bufferSize];
                int len = audioRecord.read(buffer, 0, bufferSize);
                if (len > 0) {
                   // builder.getNativePush().sendAudio(buffer, 0, len);
                }
            }
        }
    }
}
