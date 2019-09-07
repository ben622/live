package com.ben.android.live.view;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;

import com.ben.android.live.R;
import com.ben.android.live.widgets.PeriscopeLayout;
import com.ben.livesdk.widgets.LiveView;

public class LiveActivity extends AppCompatActivity {
    private LiveView mLiveView;
    private PeriscopeLayout periscopeLayout;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_live);
        mLiveView = findViewById(R.id.mLiveView);
        periscopeLayout = findViewById(R.id.mPeriscopeLayout);
        mLiveView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                periscopeLayout.addHeart();
            }
        });
        mLiveView.startPush();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mLiveView.stopPush();
        mLiveView.free();
    }
}
