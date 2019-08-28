//
// Created by ICE on 2019-08-13.
//

#include "IceFFmpeg.h"

IceFFmpeg::IceFFmpeg(JavaCallHelper *javaCallHelper, char *dataSource) {
    this->javaCallHelper = javaCallHelper;

    this->dataSource = new char[strlen(dataSource) + 1];
    strcpy(this->dataSource, dataSource);
}

IceFFmpeg::~IceFFmpeg() {
    DELETE(dataSource);
    DELETE(javaCallHelper);
}

/**
 * 准备线程pid_prepare真正执行的函数
 * @param args
 * @return
 */
void *task_prepare(void *args) {
    //打开输入
    IceFFmpeg *fFmpeg = static_cast<IceFFmpeg *>(args);
    fFmpeg->_prepare();
    //一定一定一定要返回0！！！
    return 0;
}

void *task_start(void *args) {
    IceFFmpeg *fFmpeg = static_cast<IceFFmpeg *>(args);
    fFmpeg->_start();
    return 0;
}

void *task_stop(void *args) {
    IceFFmpeg *fFmpeg = static_cast<IceFFmpeg *>(args);
    fFmpeg->isPlaying =0;
    //要保证_prepare方法(子线程中)执行完在释放
    //pthread_join:调用后会阻塞线程，所以放到子线程中调用
    pthread_join(fFmpeg->pid_prepare,0);

    if (fFmpeg->formatContext) {
        avformat_close_input(&fFmpeg->formatContext);
        avformat_free_context(fFmpeg->formatContext);
        fFmpeg->formatContext = 0;
    }

    DELETE(fFmpeg->videoChannel)
    DELETE(fFmpeg->audioChannel)
    DELETE(fFmpeg)

    return 0;
}

void IceFFmpeg::_prepare() {

    formatContext = avformat_alloc_context();

    AVDictionary *dictionary = 0;
    av_dict_set(&dictionary, "timeout", "10000000", 0);
    //1、打开媒体
    int ret = avformat_open_input(&formatContext, dataSource, 0, &dictionary);
    if (ret) {
        LOGE("打开媒体失败：%s", av_err2str(ret));
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    //2 查找媒体中的流信息
    ret = avformat_find_stream_info(formatContext, 0);
    if (ret < 0) {
        LOGE("查找媒体中的流信息失败：%s", av_err2str(ret));
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    //这里的 i 就是后面 166行的 packet->stream_index
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        //3获取媒体流（音频或视频）
        AVStream *stream = formatContext->streams[i];
        //4获取编解码这段流的参数
        AVCodecParameters *codecParameters = stream->codecpar;
        //5 通过参数中的id（编解码的方式），来查找当前流的解码器
        AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
        if (!codec) {
            LOGE("查找当前流的解码器失败");
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }
        //6 创建解码器上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            LOGE("创建解码器上下文失败");
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
        }

        //7 设置解码器上下文的参数
        ret = avcodec_parameters_to_context(codecContext, codecParameters);
        if (ret < 0) {
            LOGE("设置解码器上下文的参数失败：%s", av_err2str(ret));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        //8 打开解码器
        ret = avcodec_open2(codecContext, codec, 0);
        if (ret) {
            LOGE("打开解码器失败：%s", av_err2str(ret));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }
        AVRational time_base = stream->time_base;
        //判断流类型（音频还是视频？）
        if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            //音频
            audioChannel = new AudioChannel(i, codecContext,time_base,javaCallHelper);
        } else if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            //视频
            AVRational fram_rate = stream->avg_frame_rate;
            int fps = av_q2d(fram_rate);
            videoChannel = new VideoChannel(i, codecContext, fps,time_base,javaCallHelper);
            videoChannel->setRenderCallback(renderCallback);
        }
    }

    if (!audioChannel && !videoChannel) {
        //既没有音频也没有视频
        LOGE("没有音视频");
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }
    //准备好了，反射通知java
    if (javaCallHelper) {
        javaCallHelper->onPrepared(THREAD_CHILD);
    }
}

/**
 * 播放准备
 * 可能是主线程
 */
void IceFFmpeg::prepare() {
    pthread_create(&pid_prepare, 0, task_prepare, this);
}

/**
 * 开始播放
 */
void IceFFmpeg::start() {
    LOGD("Native start()");

    isPlaying = 1;
    if (videoChannel) {
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->start();
    }

    if (audioChannel) {
        audioChannel->start();
    }

    pthread_create(&pid_start, 0, task_start, this);
}



/**
 * 真正执行解码播放
 */
void IceFFmpeg::_start() {
    LOGD("Native _start()");
    while (isPlaying) {

        //控制packets队列大小 防止内存溢出
        if (videoChannel && videoChannel->packets.size() > 100) {
            av_usleep(10 * 1000);
            continue;
        }

        AVPacket *packet = av_packet_alloc();

        int ret = av_read_frame(formatContext, packet);

        if (!ret) {
            //ret 为0表示成功
            //要判断流类型，是视频还是音频
            if (videoChannel && packet->stream_index == videoChannel->id) {
                //往视频编码数据包队列中添加数据
                videoChannel->packets.push(packet);
            } else if (audioChannel && packet->stream_index == audioChannel->id) {
                //往音频编码数据包队列中添加数据
                audioChannel->packets.push(packet);

            }
        } else if (ret == AVERROR_EOF) {
            //表示读完了
            //要考虑读完了，是否播完了的情况
            if (videoChannel->packets.empty() && videoChannel->frames.empty() && audioChannel->packets.empty()&&audioChannel->frames.empty()) {
                av_packet_free(&packet);
                break;
            }
        } else {
            LOGE("读取音频数据包失败");
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_READ_PACKETS_FAIL);
            }
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

void IceFFmpeg::stop() {
    //prepare 阻塞中停止了，还是会回调给java 准备好了
    javaCallHelper = 0;

    pthread_create(&pid_top,0,task_stop,this);
}
