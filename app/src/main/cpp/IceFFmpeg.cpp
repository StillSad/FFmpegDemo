//
// Created by ICE on 2019-08-13.
//

#include "IceFFmpeg.h"

IceFFmpeg::IceFFmpeg(JavaCallHelper *javaCallHelper, char *dataSource) {
    this->javaCallHelper = javaCallHelper;

    this->dataSource = new char[strlen(dataSource) + 1];
    strcpy(this->dataSource,dataSource);
}

IceFFmpeg::~IceFFmpeg() {
    DELETE(dataSource);
    DELETE(javaCallHelper);
}

void *task_prepare(void *args){
    IceFFmpeg *fFmpeg = static_cast<IceFFmpeg *>(args);

    fFmpeg->_prepare();
    return 0;
}

void *task_start(void *args){
    IceFFmpeg *fFmpeg = static_cast<IceFFmpeg *>(args);
    fFmpeg->_start();
    return 0;
}




void IceFFmpeg::_prepare() {

    formatContext = avformat_alloc_context();

    AVDictionary *dictionary = 0;
    av_dict_set(&dictionary,"timeout", "10000000", 0);

    int ret = avformat_open_input(&formatContext,dataSource,0,&dictionary);
    if(ret) {
        LOGE("打开媒体失败：%s",av_err2str(ret));
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    ret = avformat_find_stream_info(formatContext,0);
    if (ret < 0) {
        LOGE("查找媒体中的流信息失败：%s",av_err2str(ret));
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    for (int i = 0; i < formatContext->nb_streams; ++i) {

        AVStream *stream = formatContext->streams[i];

        AVCodecParameters *codecParameters = stream->codecpar;

        AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);

        if (!codec) {
            LOGE("查找当前流的解码器失败");
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD,FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }

        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            LOGE("创建解码器上下文失败");
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
        }

        ret = avcodec_parameters_to_context(codecContext,codecParameters);
        if (ret < 0) {
            LOGE("设置解码器上下文的参数失败：%s",av_err2str(ret));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD,FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        ret = avcodec_open2(codecContext,codec,0);
        if (ret) {
            LOGE("打开解码器失败：%s",av_err2str(ret));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频
            audioChannel = new AudioChannel(i,codecContext);
        } else if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            //视频
            AVRational fram_rate = stream->avg_frame_rate;
            int fps = av_q2d(fram_rate);
            videoChannel = new VideoChannel(i,codecContext,fps);
            videoChannel->setRenderCallback(renderCallback);
        }
    }

    if (!audioChannel && !videoChannel) {
        LOGE("没有音视频");
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    if (javaCallHelper) {
        javaCallHelper->onPrepared(THREAD_CHILD);
    }
}

void IceFFmpeg::prepare() {
    pthread_create(&pid_prepare,0,task_prepare,this);
}

void IceFFmpeg::start() {
    LOGD("Native start()");

    isPlaying =1;
    if(videoChannel) {
        videoChannel->start();
    }

    if (audioChannel) {
        audioChannel->start();
    }

    pthread_create(&pid_start,0,task_start,this);
}

void IceFFmpeg::_start() {
    LOGD("Native _start()");
    while (isPlaying) {

        //控制packets队列大小 防止内存溢出
        if (videoChannel && videoChannel->packets.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }

        AVPacket *packet = av_packet_alloc();

        int ret = av_read_frame(formatContext,packet);

        if (!ret) {
            if (videoChannel && packet->stream_index == videoChannel->id) {
                videoChannel->packets.push(packet);
                LOGD("视频入队列");
            } else if (audioChannel && packet->stream_index == audioChannel->id) {
                LOGD("音频入队列");
                audioChannel->packets.push(packet);

            }
        } else if(ret == AVERROR_EOF) {

        } else {
            LOGE("读取音频数据包失败");
            break;
        }
    }

    isPlaying = 0;
    //停止解码播放（音频和视频）
    videoChannel->stop();
    audioChannel->stop();
}

void IceFFmpeg::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}