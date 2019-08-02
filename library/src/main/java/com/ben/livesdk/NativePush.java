package com.ben.livesdk;

public class NativePush {
    public native void setNativeVideoOptions(int width,int height,int bitrate,int fps);
    public native void setNativeAudioOptions(int sampleRateInHz,int channel);
    public native void sendVideo(byte[] data);
    public native void sendAudio(byte[] audioData, int offsetInBytes, int sizeInBytes);
    public native void prepare();
    public native void startPush();
    public native void pausePush();
    public native void stopPush();
    public native void free();

    public native void test();

}
