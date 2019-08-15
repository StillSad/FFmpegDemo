#include <jni.h>
#include <string>
#include "JavaCallHelper.h"
#include "IceFFmpeg.h"

extern "C" {
#include <libavutil/imgutils.h>

};

JavaVM *javaVM = 0;
JavaCallHelper *javaCallHelper = 0;
IceFFmpeg *fFmpeg = 0;


jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    return JNI_VERSION_1_4;
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
    fFmpeg->prepare();

    env->ReleaseStringUTFChars(dataSource_, dataSource);
}extern "C"
JNIEXPORT void JNICALL
Java_com_ice_ffmpegdemo_ICEPlayer_startNative(JNIEnv *env, jobject instance) {
    if (fFmpeg) {
        fFmpeg->start();
    }

}