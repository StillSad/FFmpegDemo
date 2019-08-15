//
// Created by ICE on 2019-08-13.
//

#ifndef FFMPEGDEMO_JAVACALLHELPER_H
#define FFMPEGDEMO_JAVACALLHELPER_H

#include <jni.h>
#include "macro.h"

class JavaCallHelper {
public:
    JavaCallHelper(JavaVM *javaVM_,JNIEnv *env_,jobject instance_);

    ~JavaCallHelper();

    void onPrepared(int threadMode);

private:
    JavaVM *javaVM;
    JNIEnv *env;
    jobject instance;
    jmethodID jmd_prepared;
};


#endif //FFMPEGDEMO_JAVACALLHELPER_H
