package com.ben.livesdk.api;

import android.content.Context;


/**
 * @author @zhangchuan622@gmail.com
 * @version 1.0 SDK初始化
 * @create 2019/7/30
 */
public class SDKInitializer {

    public static void init(Context context) {
        System.loadLibrary("live");
    }

}
