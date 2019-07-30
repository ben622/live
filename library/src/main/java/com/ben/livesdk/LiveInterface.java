package com.ben.livesdk;

public interface LiveInterface {
    void prepare();
    void startPush();
    void pausePush();
    void stopPush();
    void free();
}
