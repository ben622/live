package com.ben.android.live;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.media.Image;
import android.util.Log;
import android.view.SurfaceHolder;

import java.util.List;


public class VideoPusher extends APusher implements SurfaceHolder.Callback,VideoPushInterface, Camera.PreviewCallback {
    private Builder builder;
    private Camera mCamera;
    private byte[] callbackBuffer;

    public Builder getBuilder() {
        return builder;
    }

    public VideoPusher(Builder builder) {
        this.builder = builder;

    }

    @Override
    public void prepare() {
        builder.surfaceHolder.addCallback(this);
    }

    @Override
    public void startPush() {
        isPushing = true;
        builder.getNativePush().setNativeVideoOptions(builder.getWidth(),builder.getHeight(),builder.getBitrate(),builder.getFps());
        builder.getNativePush().startPush();
    }

    @Override
    public void pausePush() {
        isPushing = false;
        builder.getNativePush().pausePush();
    }

    @Override
    public void stopPush() {
        isPushing = false;
        builder.getNativePush().stopPush();

    }

    @Override
    public void free() {
        stopPush();
        stopPreview();
        builder.getNativePush().free();

    }


    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        startPreview();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    public void stopPreview() {
        if (mCamera != null) {
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
        }
    }

    @Override
    public void startPreview() {
        //当surfaceView被创建的时候启动摄像头预览
        try {
            mCamera = Camera.open(builder.getCameraId());
            Camera.Parameters parameters = mCamera.getParameters();
            List<Camera.Size> sizeList = parameters.getSupportedPreviewSizes();

            parameters.setPreviewFormat(ImageFormat.NV21);
//            parameters.setPreviewFpsRange(24,25);
            parameters.setPictureSize(builder.getWidth(),builder.getHeight());
            parameters.setPreviewSize(builder.getWidth(),builder.getHeight());
            mCamera.setParameters(parameters);
            mCamera.setPreviewDisplay(builder.getSurfaceHolder());
            mCamera.setDisplayOrientation(90);
            //ARGB 8888 1pix 4字节
            callbackBuffer = new byte[builder.getWidth() * builder.getHeight() * 4];
            mCamera.addCallbackBuffer(callbackBuffer);
            mCamera.setPreviewCallbackWithBuffer(this);

            mCamera.startPreview();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    public void switchCamera() {
        stopPreview();
        startPreview();
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mCamera != null) {
            mCamera.addCallbackBuffer(callbackBuffer);
        }

        if (isPushing) {
            builder.getNativePush().sendVideo(data);
        }


    }

    public static class Builder {
        private int width;
        private int height;
        private int cameraId;
        private int bitrate = 480000;
        private int fps = 25;
        private NativePush nativePush;
        private SurfaceHolder surfaceHolder;

        public int getWidth() {
            return width;
        }

        public Builder width(int width) {
            this.width = width;
            return this;
        }

        public int getHeight() {
            return height;
        }

        public Builder height(int height) {
            this.height = height;
            return this;
        }

        public SurfaceHolder getSurfaceHolder() {
            return surfaceHolder;
        }

        public Builder surfaceView(SurfaceHolder surfaceHolder) {
            this.surfaceHolder = surfaceHolder;
            return this;
        }

        public int getCameraId() {
            return cameraId;
        }

        public Builder cameraId(int cameraId) {
            this.cameraId = cameraId;
            return this;
        }

        public int getBitrate() {
            return bitrate;
        }

        public Builder bitrate(int bitrate) {
            this.bitrate = bitrate;
            return this;
        }

        public int getFps() {
            return fps;
        }

        public Builder fps(int fps) {
            this.fps = fps;
            return this;
        }

        public NativePush getNativePush() {
            return nativePush;
        }

        public Builder nativePush(NativePush nativePush) {
            this.nativePush = nativePush;
            return this;
        }
        public VideoPusher build() {
            return new VideoPusher(this);
        }


    }



}
