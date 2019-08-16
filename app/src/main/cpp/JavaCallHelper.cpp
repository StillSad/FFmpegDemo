//
// Created by ICE on 2019-08-13.
//

#include "JavaCallHelper.h"


JavaCallHelper::JavaCallHelper(JavaVM *javaVM_, JNIEnv *env_, jobject instance_) {
    this->javaVM = javaVM_;
    this->env = env_;
    this->instance = env->NewGlobalRef(instance_);
    jclass clazz = env->GetObjectClass(instance);

    jmd_prepared = env->GetMethodID(clazz,"onPrepared","()V");
    jmd_onError = env->GetMethodID(clazz,"onError","(I)V");

}

JavaCallHelper::~JavaCallHelper() {
    javaVM = 0;
    env->DeleteGlobalRef(instance);
    instance = 0;
}

void JavaCallHelper::onPrepared(int threadMode) {
    if(threadMode == THREAD_MAIN) {
        //主线程
        env->CallVoidMethod(instance,jmd_prepared);
    } else {
        //子线程
        //当前子线程的 JNIEnv
        JNIEnv *env_child;
        javaVM->AttachCurrentThread(&env_child,0);
        env_child->CallVoidMethod(instance,jmd_prepared);
        javaVM->DetachCurrentThread();
    }
}

void JavaCallHelper::onError(int threadMode, int errorCode) {
    if (threadMode == THREAD_MAIN) {
        env->CallVoidMethod(instance,jmd_onError,errorCode);
    } else {
        JNIEnv *env_child;
        javaVM->AttachCurrentThread(&env_child,0);
        env_child->CallVoidMethod(instance,jmd_onError,errorCode);
        javaVM->DetachCurrentThread();
    }
}