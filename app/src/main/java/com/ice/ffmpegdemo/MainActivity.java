package com.ice.ffmpegdemo;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    private SurfaceView surfaceView;
    private ICEPlayer player;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        surfaceView = findViewById(R.id.surfaceView);

        player = new ICEPlayer();
        player.setSurfaceView(surfaceView);
        player.setDataSource(new File(
                Environment.getExternalStorageDirectory() + File.separator + "input.mp4").getAbsolutePath());
        player.setOnpreparedListener(new ICEPlayer.OnpreparedListener() {
            @Override
            public void onPrePared() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Log.e(TAG,"开始播放");
                        Toast.makeText(MainActivity.this,"开始播放！",Toast.LENGTH_SHORT).show();
                    }
                });

                player.start();
            }
        });

        player.setOnErrorListener(new ICEPlayer.OnErrorListener() {
            @Override
            public void onError(final int errorCode) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity.this, "出错了，错误码：" + errorCode,
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        player.prepare();
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}
