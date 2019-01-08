package com.ben.android.live;

public interface LiveInterface {
    void prepare();
    void startPush();
    void pausePush();
    void stopPush();
    void free();
}
