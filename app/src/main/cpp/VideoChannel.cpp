//
// Created by ICE on 2019-08-13.
//

#include "VideoChannel.h"


/**
 * 丢包（AVPacket）
 * @param q
 */
void dropAVPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        AVPacket *avPacket = q.front();

        if (avPacket->flags != AV_PKT_FLAG_KEY) {
            BaseChannel::releaseAVPacket(&avPacket);
            q.pop();
        } else {
            break;
        }
    }
}

/**
 * 丢包（AVFrame）
 * @param q
 */
 void dropAVFrame(queue<AVFrame *> &q) {
     if (!q.empty()) {
         AVFrame *avFrame = q.front();
         BaseChannel::releaseAVFrame(&avFrame);
         q.pop();
     }
 }

VideoChannel::VideoChannel(int id, AVCodecContext *codecContext, int fps, AVRational time_base)
        : BaseChannel(id,codecContext,time_base) {
    this->fps = fps;
    packets.setSyncHandle(dropAVPacket);
    frames.setSyncHandle(dropAVFrame);
}

VideoChannel::~VideoChannel() {

}

void *task_video_decode(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    //dataSource
    videoChannel->video_decode();
    return 0;
}


void *task_video_play(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    //2 dataSource
    videoChannel->video_play();
    return 0;
}


void VideoChannel::start() {
    LOGD("VideoChannel start()");

    isPlaying = 1;
    //设置队列状态为工作状态
    packets.setWork(1);
    frames.setWork(1);
    //解码
    pthread_create(&pid_video_decode, 0, task_video_decode, this);
    //播放
    pthread_create(&pid_video_play, 0, task_video_play, this);

}

void VideoChannel::stop() {

}


/**
 * 真正视频解码
 */
void VideoChannel::video_decode() {
    LOGD("VideoChannel 视频解码");
    AVPacket *packet = 0;
    while (isPlaying) {
        int ret = packets.pop(packet);
        if (!isPlaying) {
            //如果停止播放了，跳出循环 释放packet
            break;
        }

        if (!ret) {
            //取数据包失败
            continue;
        }
        //拿到了视频数据包（编码压缩了的）,需要报数据包给解码器进行解码
        ret = avcodec_send_packet(codecContext, packet);

        if (ret) {
            //往解码器发送数据包失败，跳出循环
            ;
        }
        //释放packet
        releaseAVPacket(&packet);

        AVFrame *frame = av_frame_alloc();

        ret = avcodec_receive_frame(codecContext, frame);

        if (ret == AVERROR(EAGAIN)) {
            //重来
            continue;
        } else if (ret != 0) {
            break;
        }
        //控制frames大小 防止内存溢出
        while (isPlaying && frames.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }
        //ret == 0 数据收发正常,成功获取到了解码后的视频原始数据包 AVFrame ，格式是 yuv
        //对frame进行处理（渲染播放）直接写？
        frames.push(frame);
    }

    releaseAVPacket(&packet);
}

void VideoChannel::video_play() {
    LOGD("VideoChannel 视频播放");

    AVFrame *frame = 0;

    uint8_t *dst_data[4];

    int dst_linesize[4];
    //yuv转rgba上下文
    SwsContext *sws_ctx = sws_getContext(codecContext->width, codecContext->height,
                                         codecContext->pix_fmt,
                                         codecContext->width, codecContext->height, AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, NULL, NULL, NULL);
    //给dst_data dst_linesize 申请内存
    av_image_alloc(dst_data, dst_linesize,
                   codecContext->width, codecContext->height, AV_PIX_FMT_RGBA, 1);
    //根据fps（传入的流的平均帧率)来控制每一帧的延时时间
    //单位秒
    double delay_time_per_frame = 1.0 / fps;

    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }

        if (!ret) {
            continue;
        }

        //取到了yuv原始数据，下面要进行格式转换
        sws_scale(sws_ctx, frame->data,
                  frame->linesize, 0, codecContext->height, dst_data, dst_linesize);

        //进行休眠
        //每一帧还有自己的额外延时时间
        double extra_delay = frame->repeat_pict / (2 * fps);
        double real_delay = delay_time_per_frame + extra_delay;

        double video_time = frame->best_effort_timestamp * av_q2d(time_base);

        //音视频同步：永远都是你追我赶的状态
        if (!audioChannel) {
            //没有音频（类似GIF）
            //单位是微妙
            av_usleep(real_delay * 1000000);
        } else {
            double audioTime = audioChannel->audio_time;
            //获取音视频播放的时间差
            double  time_diff = video_time - audioTime;
            if (time_diff > 0) {
                LOGD("视频比音频快：%lf", time_diff);
                //视频比音频快：等音频
                //自然播放状态下，time_diff的值不会很大
                //但是，seek后time_diff的值可能会很大，导致视频休眠太久

                if (time_diff > 1) {
                    av_usleep(real_delay * 2 * 1000000);
                } else {
                    av_usleep((real_delay + time_diff) * 1000000);
                }
            } else if(time_diff < 0) {
                LOGE("音频比视频快: %lf", fabs(time_diff));
                //音频比视频快：追音频(尝试丢视频包)
                //视频包：packets 和 frames
                if (fabs(time_diff) >= 0.05) {
                    //时间差如果大于0.05，有明显的延迟感
                    //丢包：要操作队列中数据！一定要小心
                    frames.sync();
                    continue;
                }
            } else {
                LOGE("音视频完美同步！");
            }
        }

        //dst_data：AV_PIX_FMT_RGBA格式的数据
        //渲染
        renderCallback(dst_data[0], dst_linesize[0], codecContext->width, codecContext->height);
        releaseAVFrame(&frame);
    }

    releaseAVFrame(&frame);
    isPlaying = 0;
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
}

void VideoChannel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audioChannel = audioChannel;
}