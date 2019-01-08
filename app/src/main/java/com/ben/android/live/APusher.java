package com.ben.android.live;

public abstract class APusher implements LiveInterface{
    protected String TAG = this.getClass().getSimpleName();
    protected boolean isPushing = false;

}
