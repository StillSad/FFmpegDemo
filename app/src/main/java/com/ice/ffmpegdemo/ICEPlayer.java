package com.ice.ffmpegdemo;

public class ICEPlayer {

    static {
        System.loadLibrary("native-lib");
    }

    private String dataSource;

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    public void prepare(){
        prepareNative(dataSource);
    }

    public void start() {
        startNative();
    }

    public void onPrepared() {
        if (onpreparedListener != null) {
            onpreparedListener.onPrePared();
        }
    }

    void setOnpreparedListener(OnpreparedListener onpreparedListener) {
        this.onpreparedListener = onpreparedListener;
    }

    interface OnpreparedListener {
        void onPrePared();
    }

    private OnpreparedListener onpreparedListener;

    private native void prepareNative(String dataSource);

    private native void startNative();
}
