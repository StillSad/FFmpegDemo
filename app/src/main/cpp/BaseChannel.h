//
// Created by ICE on 2019-08-13.
//

#ifndef FFMPEGDEMO_BASECHANNEL_H
#define FFMPEGDEMO_BASECHANNEL_H

#include "safe_queue.h"
#include "JavaCallHelper.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
};

class BaseChannel {
public:


    BaseChannel(int id,AVCodecContext *codecContext,AVRational time_base,JavaCallHelper *javaCallHelper):id(id),codecContext(codecContext),time_base(time_base),javaCallHelper(javaCallHelper)
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
    int id;
    bool isPlaying = 0;
    AVCodecContext *codecContext;
    AVRational time_base;
    double audio_time;
    JavaCallHelper *javaCallHelper = 0;
};

#endif //FFMPEGDEMO_BASECHANNEL_H

