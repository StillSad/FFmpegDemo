#include <jni.h>
#include <string>
#include "JavaCallHelper.h"
#include "IceFFmpeg.h"
#include <android/native_window_jni.h>
extern "C" {
#include <libavutil/imgutils.h>
};

JavaVM *javaVM = 0;
JavaCallHelper *javaCallHelper = 0;
IceFFmpeg *fFmpeg = 0;
ANativeWindow *window = 0;
//静态初始化mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    return JNI_VERSION_1_4;
}

//1，data;2，linesize；3，width; 4， height
void renderFrame(uint8_t *src_data,int src_lineSize,int width,int height) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    ANativeWindow_setBuffersGeometry(window,width,
            height,WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;
    if (ANativeWindow_lock(window,&windowBuffer,0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }

    //把buffer中的数据进行赋值（修改）
    uint8_t  *dst_data = static_cast<uint8_t *>(windowBuffer.bits);
    int dst_lineSize = windowBuffer.stride * 4;
    //逐行拷贝
    for (int i = 0; i < windowBuffer.height; ++i) {
        memcpy(dst_data + i* dst_lineSize,src_data + i * src_lineSize,dst_lineSize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}



extern "C" JNIEXPORT jstring JNICALL
Java_com_ice_ffmpegdemo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(av_version_info());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_ice_ffmpegdemo_ICEPlayer_prepareNative(JNIEnv *env, jobject instance,
                                                jstring dataSource_) {
    const char *dataSource = env->GetStringUTFChars(dataSource_, 0);

    javaCallHelper = new JavaCallHelper(javaVM,env,instance);
    fFmpeg = new IceFFmpeg(javaCallHelper, const_cast<char *>(dataSource));
    fFmpeg->setRenderCallback(renderFrame);
    fFmpeg->prepare();

    env->ReleaseStringUTFChars(dataSource_, dataSource);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ice_ffmpegdemo_ICEPlayer_startNative(JNIEnv *env, jobject instance) {
    if (fFmpeg) {
        fFmpeg->start();
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_ice_ffmpegdemo_ICEPlayer_setSurfaceViewNative(JNIEnv *env, jobject thiz, jobject surface) {
    pthread_mutex_lock(&mutex);

    if(window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env,surface);

    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ice_ffmpegdemo_ICEPlayer_stopNative(JNIEnv *env, jobject thiz) {
  if (fFmpeg) {
      fFmpeg->stop();
  }
}



extern "C"
JNIEXPORT void JNICALL
Java_com_ice_ffmpegdemo_ICEPlayer_releaseNative(JNIEnv *env, jobject thiz) {

    pthread_mutex_lock(&mutex);
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    pthread_mutex_unlock(&mutex);
    DELETE(fFmpeg)
}



extern "C"
JNIEXPORT jint JNICALL
Java_com_ice_ffmpegdemo_ICEPlayer_getDurationNative(JNIEnv *env, jobject thiz) {
    if (fFmpeg) {
        return fFmpeg->getDuration();
    }
    return 0;
}extern "C"
JNIEXPORT void JNICALL
Java_com_ice_ffmpegdemo_ICEPlayer_seekToNative(JNIEnv *env, jobject thiz, jint play_progress) {
    if (fFmpeg) {
        fFmpeg->seekTo(play_progress);
    }
}