//
// Created by ICE on 2019-08-13.
//

#ifndef FFMPEGDEMO_BASECHANNEL_H
#define FFMPEGDEMO_BASECHANNEL_H

#include "safe_queue.h"

extern "C"{
#include "libavcodec/avcodec.h"
};

class BaseChannel {
public:
    int id;

    BaseChannel(int id):id(id)
    {
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);
    }
    virtual ~BaseChannel()
    {
        packets.clear();
        frames.clear();
    }

    static void releaseAVPacket(AVPacket **packet){
        if (packet) {
            av_packet_free(packet);
            *packet = 0;
        }
    }

    static void releaseAVFrame(AVFrame **frame) {
        if (frame) {
            av_frame_free(frame);
            *frame = 0;
        }
    }

    virtual void start() = 0;
    virtual void stop() = 0;

    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
};

#endif //FFMPEGDEMO_BASECHANNEL_H

