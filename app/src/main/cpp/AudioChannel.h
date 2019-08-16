//
// Created by ICE on 2019-08-13.
//


#ifndef FFMPEGDEMO_AUDIOCHANNEL_H
#define FFMPEGDEMO_AUDIOCHANNEL_H

#include "BaseChannel.h"

class AudioChannel: public BaseChannel{
public:
    AudioChannel(int id,AVCodecContext *codecContext);

    ~AudioChannel();

    void start();

    void stop();

};


#endif //FFMPEGDEMO_AUDIOCHANNEL_H

