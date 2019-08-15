//
// Created by ICE on 2019-08-14.
//

#ifndef FFMPEGDEMO_MACRO_H
#define FFMPEGDEMO_MACRO_H

#include <android/log.h>
//定义释放的宏函数
#define DELETE(object) if(object){delete object;object = 0;}

//定义日志打印宏函数
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"ICEFFMPEG",__VA_ARGS__)

//标记线程模式
#define THREAD_MAIN 1
#define THREAD_CHILD 2


#endif //FFMPEGDEMO_MACRO_H
