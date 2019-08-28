package com.ice.ffmpegdemo;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class ICEPlayer implements SurfaceHolder.Callback {

    static {
        System.loadLibrary("native-lib");
    }

    //准备过程错误码
    public static final int ERROR_CODE_FFMPEG_PREPARE = -1000;
    //播放过程错误码
    public static final int ERROR_CODE_FFMPEG_PLAY = -2000;
    //打不开视频
    public static final int FFMPEG_CAN_NOT_OPEN_URL = (ERROR_CODE_FFMPEG_PREPARE - 1);
    //找不到媒体流信息
    public static final int FFMPEG_CAN_NOT_FIND_STREAM = (ERROR_CODE_FFMPEG_PREPARE - 2);
    //找不到解码器
    public static final int FFMPEG_FIND_DECODER_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 3);
    //无法根据解码器创建上下文
    public static final int FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 5);
    //根据刘信息配置上下文参数失败
    public static final int FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 5);
    //打开解码器失败
    public static final int FFMPEG_OPEN_DECODER_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 6);
    //没有音视频
    public static final int FFMPEG_NO_MEDIA = (ERROR_CODE_FFMPEG_PREPARE - 7);
    //读取媒体数据包失败
    public static final int FFMPEG_READ_PACKETS_FAIL = (ERROR_CODE_FFMPEG_PLAY - 1);
    //直播地址或媒体文件路径
    private String dataSource;
    private SurfaceHolder surfaceHolder;

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    /**
     * 播放准备工作
     */
    public void prepare() {
        prepareNative(dataSource);
    }

    /**
     * 开始播放
     */
    public void start() {
        startNative();
    }

    /**
     * 供native反射调用
     * 表示播放器准备好了可以开始播放
     */
    public void onPrepared() {
        if (playerListener != null) {
            playerListener.onPrepared();
        }
    }

    public void onError(int errorCode) {
        if (null != playerListener) {
            playerListener.onError(errorCode);
        }
    }

    public void onProgress(int progress) {
        if (null != playerListener) {
            playerListener.onProgress(progress);
        }
    }

    public void setPlayerListener(PlayerListener playerListener) {
        this.playerListener = playerListener;
    }


    public void setSurfaceView(SurfaceView surfaceView) {
        if (null != surfaceHolder) {
            surfaceHolder.removeCallback(this);
        }
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
    }

    /**
     * 画布创建回调
     *
     * @param holder
     */
    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    /**
     * 画布刷新
     *
     * @param holder
     * @param format
     * @param width
     * @param height
     */
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        setSurfaceViewNative(holder.getSurface());
    }

    /**
     * 画布销毁
     *
     * @param holder
     */
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void release() {
        surfaceHolder.removeCallback(this);
        releaseNative();
    }

    /**
     * 停止播放
     */
    public void stop() {
        stopNative();
    }


    /**
     * 获取总的播放时长
     */
    public int getDuration() {
        return getDurationNative();
    }

    interface OnpreparedListener {

    }

    public interface PlayerListener {
        /**
         * 画布创建回调
         */
        void onPrepared();

        /**
         * 出错回调
         * @param errorCode 错误码
         */
        void onError(int errorCode);

        /**
         * 播放进度回调
         * @param progress 进度
         */
        void onProgress(int progress);

    }

    private PlayerListener playerListener;

    private native void prepareNative(String dataSource);

    private native void startNative();

    private native void setSurfaceViewNative(Surface surface);

    private native void stopNative();

    private native void releaseNative();

    private native int getDurationNative();
}
