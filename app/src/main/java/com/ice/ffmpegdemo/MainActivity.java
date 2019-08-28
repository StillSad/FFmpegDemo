package com.ice.ffmpegdemo;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity implements ICEPlayer.PlayerListener, SeekBar.OnSeekBarChangeListener, View.OnTouchListener {
    private static final String TAG = "MainActivity";
    private SurfaceView surfaceView;
    private SeekBar seekBar;
    private ICEPlayer player;
    private boolean isSeek;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        surfaceView = findViewById(R.id.surfaceView);
        seekBar = findViewById(R.id.seekBar);
        seekBar.setOnSeekBarChangeListener(this);
        seekBar.setOnTouchListener(this);
        player = new ICEPlayer();
        player.setSurfaceView(surfaceView);
//        String dataSource = "http://ivi.bupt.edu.cn/hls/cctv6hd.m3u8";
        String dataSource = new File(Environment.getExternalStorageDirectory() + File.separator + "input.mp4").getAbsolutePath();
        player.setDataSource(dataSource);

        player.setPlayerListener(this);


    }

    @Override
    protected void onResume() {
        super.onResume();
        int flag = ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE);
        if (flag == PackageManager.PERMISSION_GRANTED) {
            player.prepare();
        } else {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1001);
        }

    }


    @Override
    protected void onPause() {
        super.onPause();
        player.stop();
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    @Override
    public void onPrepared() {
        int duration = player.getDuration();
        if (duration != 0) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    seekBar.setVisibility(View.VISIBLE);
                }
            });
        }
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.e(TAG, "开始播放");
                Toast.makeText(MainActivity.this, "开始播放！", Toast.LENGTH_SHORT).show();
            }
        });

        player.start();
    }

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

    @Override
    public void onProgress(final int progress) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                int duration = player.getDuration();
                Log.d("seek", "progress：" + progress + ",duration:" + duration);

                if (duration != 0 && !isSeek) {
                    seekBar.setProgress(progress * 100 / duration);
                }
            }
        });
    }


    //seek的核心思路
    //跟随播放进度自动刷新进度：拿到每个时间点相对总播放时长的百分比进度 progress
    //1.总时间getDurationNative
    //2.当前播放时间：跟谁播放进度动态变化
    @Override
    public void onProgressChanged(final SeekBar seekBar, final int progress, boolean fromUser) {
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        //获取seekbar当前进度（百分比）
        int seekBarProgress = seekBar.getProgress();
        //将seekbar的进度转换成真实的播放进度
        int duration = player.getDuration();
        int playProgress = seekBarProgress * duration / 100;
        //将播放进度传给底层ffmpeg
        player.seekTo(playProgress);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.release();
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                isSeek = true;

                break;
            case MotionEvent.ACTION_UP:
                isSeek = false;

                break;
        }

        return false;
    }
}
