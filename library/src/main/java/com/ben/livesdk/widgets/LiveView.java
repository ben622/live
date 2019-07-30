package com.ben.livesdk.widgets;

import android.content.Context;
import android.util.AttributeSet;
import android.view.SurfaceView;

import com.ben.livesdk.LiveInterface;
import com.ben.livesdk.LiveManager;


public class LiveView extends SurfaceView implements LiveInterface {

    private LiveManager liveManager;

    public LiveView(Context context) {
        this(context,null);
    }

    public LiveView(Context context, AttributeSet attrs) {
        this(context, attrs,0);
    }

    public LiveView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        init();
    }

    private void init() {
        liveManager = LiveManager.getManager(getContext(),this);
        prepare();
    }


    @Override
    public void prepare() {
        liveManager.prepare();
    }

    @Override
    public void startPush() {
        liveManager.startPush();
    }

    @Override
    public void pausePush() {
        liveManager.pausePush();
    }

    @Override
    public void stopPush() {
        liveManager.stopPush();

    }

    @Override
    public void free() {
        liveManager.free();

    }


    public void switchCamera() {
        liveManager.switchCameraId();
    }
}
