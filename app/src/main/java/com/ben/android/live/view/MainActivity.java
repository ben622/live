package com.ben.android.live.view;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;

import com.ben.android.live.R;
import com.ben.livesdk.api.SDKInitializer;
import com.ben.livesdk.widgets.LiveView;

public class MainActivity extends AppCompatActivity {


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            String[] perms = {"android.permission.CAMERA", "android.permission.RECORD_AUDIO", "android.permission.READ_EXTERNAL_STORAGE", "android.permission.WRITE_EXTERNAL_STORAGE"};
            if (checkSelfPermission(perms[0]) == PackageManager.PERMISSION_DENIED ||
                    checkSelfPermission(perms[1]) == PackageManager.PERMISSION_DENIED ||
                    checkSelfPermission(perms[2]) == PackageManager.PERMISSION_DENIED ||
                    checkSelfPermission(perms[3]) == PackageManager.PERMISSION_DENIED) {
                requestPermissions(perms, 200);
            }
        }


        SDKInitializer.init(this, "");
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.id_start_push:
                startActivity(new Intent(this, LiveActivity.class));
                break;
            case R.id.id_test_rtmp_stream:
                startActivity(new Intent(this, PlayerActivity.class));
                break;
        }
    }
}
