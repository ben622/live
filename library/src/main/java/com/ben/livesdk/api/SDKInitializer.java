package com.ben.livesdk.api;

import android.content.Context;
import android.util.Log;

import com.ben.livesdk.utils.SignatureTypes;

import java.lang.reflect.Method;


/**
 * @author @zhangchuan622@gmail.com
 * @version 1.0 SDK初始化
 * @create 2019/7/30
 */
public class SDKInitializer {

    public static void init(Context context) {
        System.loadLibrary("live");
    }

    private native static void init();

}
