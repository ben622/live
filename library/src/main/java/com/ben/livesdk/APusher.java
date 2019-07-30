package com.ben.livesdk;

public abstract class APusher implements LiveInterface{
    protected String TAG = this.getClass().getSimpleName();
    protected boolean isPushing = false;

}
