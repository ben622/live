package com.ben.android.live.view;

import android.content.pm.PackageManager;
import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

import com.ben.android.live.R;
import com.ben.livesdk.NativePush;
import com.ben.livesdk.api.SDKInitializer;
import com.ben.livesdk.widgets.LiveView;

public class MainActivity extends AppCompatActivity {


    private LiveView mLiveView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mLiveView = findViewById(R.id.id_surfaceview);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            String[] perms = {"android.permission.CAMERA","android.permission.RECORD_AUDIO","android.permission.READ_EXTERNAL_STORAGE", "android.permission.WRITE_EXTERNAL_STORAGE"};
            if (checkSelfPermission(perms[0]) == PackageManager.PERMISSION_DENIED ||
                    checkSelfPermission(perms[1]) == PackageManager.PERMISSION_DENIED ||
                    checkSelfPermission(perms[2]) == PackageManager.PERMISSION_DENIED ||
                    checkSelfPermission(perms[3]) == PackageManager.PERMISSION_DENIED) {
                requestPermissions(perms, 200);
            }
        }

        SDKInitializer.init(this);
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.id_start_push:
                mLiveView.startPush();
                break;
            case R.id.id_stop_push:
                mLiveView.stopPush();
                break;
            case R.id._id_switch_camera:
                mLiveView.switchCamera();
                break;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mLiveView.free();
    }


}
