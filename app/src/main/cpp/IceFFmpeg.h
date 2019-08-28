//
// Created by ICE on 2019-08-13.
//

#ifndef FFMPEGDEMO_ICEFFMPEG_H
#define FFMPEGDEMO_ICEFFMPEG_H

#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"
#include <pthread.h>

extern "C" {
#include <libavformat/avformat.h>
};

class IceFFmpeg {
    friend void *task_stop(void *args);
public:
    IceFFmpeg(JavaCallHelper *javaCallHelper,char *dataSource);

    ~IceFFmpeg();

    void prepare();

    void _prepare();

    void start();

    void _start();

    void setRenderCallback(RenderCallback renderCallback);

    void stop();

    void setDuration(int duration);

    int getDuration() const;

    void seekTo(int i);

private:
    JavaCallHelper *javaCallHelper = 0;
    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    char *dataSource;
    pthread_t pid_prepare;
    pthread_t pid_start;
    pthread_t pid_top;
    bool isPlaying;
    AVFormatContext *formatContext = 0;
    RenderCallback renderCallback;
    int duration;
    pthread_mutex_t seekMutex;

};


#endif //FFMPEGDEMO_ICEFFMPEG_H
