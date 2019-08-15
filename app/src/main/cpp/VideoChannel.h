//
// Created by ICE on 2019-08-13.
//

#ifndef FFMPEGDEMO_VIDEOCHANNEL_H
#define FFMPEGDEMO_VIDEOCHANNEL_H


#include "BaseChannel.h"

class VideoChannel: public BaseChannel {
public:
    VideoChannel(int id);

    ~VideoChannel();

    void start();

    void stop();

};


#endif //FFMPEGDEMO_VIDEOCHANNEL_H
