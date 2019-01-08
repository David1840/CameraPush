#include <jni.h>
#include<android/log.h>

#define LOG_TAG    "ffmpeg-c"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include "libavutil/time.h"
#include "libavutil/imgutils.h"


AVFormatContext *ofmt_ctx;
AVStream *video_st;
AVCodecContext *pCodecCtx;
AVCodec *pCodec;
AVPacket enc_pkt;
AVFrame *pFrameYUV;
int count = 0;
int yuv_width;
int yuv_height;
int y_length;
int uv_length;
int width = 640;
int height = 480;
int fps = 15;

/*
 * Class:     com_david_camerapush_ffmpeg_FFmpegHandler
 * Method:    init  初始化ffmpeg相关,准备推送
 * Signature: (Ljava/lang/String;)I rmtp服务地址
 */
JNIEXPORT jint JNICALL Java_com_david_camerapush_ffmpeg_FFmpegHandler_init
        (JNIEnv *jniEnv, jobject instance, jstring url) {

    const char *out_url = (*jniEnv)->GetStringUTFChars(jniEnv, url, 0);

    //计算yuv数据的长度
    yuv_width = width;
    yuv_height = height;
    y_length = width * height;
    uv_length = width * height / 4;
    //output initialize
    int ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_url);
    if (ret < 0) {
        LOGE("avformat_alloc_output_context2 error");
    }


    //output encoder initialize
    pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);

    if (!pCodec) {
        LOGE("Can not find encoder!\n");
        return -1;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    //编码器的ID号，这里为264编码器，可以根据video_st里的codecID 参数赋值
    pCodecCtx->codec_id = pCodec->id;

    //像素的格式，也就是说采用什么样的色彩空间来表明一个像素点
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    //编码器编码的数据类型
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    //编码目标的视频帧大小，以像素为单位
    pCodecCtx->width = width;
    pCodecCtx->height = height;
    pCodecCtx->framerate = (AVRational) {15, 1};

    //帧率的基本单位，我们用分数来表示，
    pCodecCtx->time_base = (AVRational) {1, 15};
    //目标的码率，即采样的码率；显然，采样码率越大，视频大小越大
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size = 50;
    /* Some formats want stream headers to be separate. */
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //H264 codec param
//    pCodecCtx->me_range = 16;
    //pCodecCtx->max_qdiff = 4;
    pCodecCtx->qcompress = 0.6;
    //最大和最小量化系数
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;
    //Optional Param
    //两个非B帧之间允许出现多少个B帧数
    //设置0表示不使用B帧，b 帧越多，图片越小
    pCodecCtx->max_b_frames = 0;
    AVDictionary *param = 0;
    //H.264
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "superfast", 0); //x264编码速度的选项
        av_dict_set(&param, "tune", "zerolatency", 0);
    }

    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
        LOGE("Failed to open encoder!\n");
        return -1;
    }

    //Add a new stream to output,should be called by the user before avformat_write_header() for muxing
    video_st = avformat_new_stream(ofmt_ctx, pCodec);
    if (video_st == NULL) {
        return -1;
    }
    video_st->time_base = (AVRational) {25, 1};
    video_st->codecpar->codec_tag = 0;
    avcodec_parameters_from_context(video_st->codecpar, pCodecCtx);

    int err = avio_open(&ofmt_ctx->pb, out_url, AVIO_FLAG_READ_WRITE);
    if (err < 0) {
        LOGE("Failed to open output：%s", av_err2str(err));
        return -1;
    }

    //Write File Header
    avformat_write_header(ofmt_ctx, NULL);
    av_init_packet(&enc_pkt);

    return 0;
}


JNIEXPORT jint JNICALL Java_com_david_camerapush_ffmpeg_FFmpegHandler_pushCameraData
        (JNIEnv *jniEnv, jobject instance, jbyteArray yArray, jint yLen, jbyteArray uArray, jint uLen,
         jbyteArray vArray, jint vLen) {
    jbyte *yin = (*jniEnv)->GetByteArrayElements(jniEnv, yArray, NULL);
    jbyte *uin = (*jniEnv)->GetByteArrayElements(jniEnv, uArray, NULL);
    jbyte *vin = (*jniEnv)->GetByteArrayElements(jniEnv, vArray, NULL);

    int ret = 0;

    pFrameYUV = av_frame_alloc();
    int picture_size = av_image_get_buffer_size(pCodecCtx->pix_fmt, pCodecCtx->width,
                                                pCodecCtx->height, 1);
    uint8_t *buffers = (uint8_t *) av_malloc(picture_size);


    //将buffers的地址赋给AVFrame中的图像数据，根据像素格式判断有几个数据指针
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, buffers, pCodecCtx->pix_fmt,
                         pCodecCtx->width, pCodecCtx->height, 1);

    memcpy(pFrameYUV->data[0], yin, (size_t) yLen); //Y
    memcpy(pFrameYUV->data[1], uin, (size_t) uLen); //U
    memcpy(pFrameYUV->data[2], vin, (size_t) vLen); //V
    pFrameYUV->pts = count;
    pFrameYUV->format = AV_PIX_FMT_YUV420P;
    pFrameYUV->width = yuv_width;
    pFrameYUV->height = yuv_height;

    //例如对于H.264来说。1个AVPacket的data通常对应一个NAL
    //初始化AVPacket
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
//    __android_log_print(ANDROID_LOG_WARN, "eric", "编码前时间:%lld",
//                        (long long) ((av_gettime() - startTime) / 1000));
    //开始编码YUV数据
    ret = avcodec_send_frame(pCodecCtx, pFrameYUV);
    if (ret != 0) {
        LOGE("avcodec_send_frame error");
        return -1;
    }
    //获取编码后的数据
    ret = avcodec_receive_packet(pCodecCtx, &enc_pkt);

    av_frame_free(&pFrameYUV);
    if (ret != 0 || enc_pkt.size <= 0) {
        LOGE("avcodec_receive_packet error %s", av_err2str(ret));
        return -2;
    }
    enc_pkt.stream_index = video_st->index;
    enc_pkt.pts = count * (video_st->time_base.den) / ((video_st->time_base.num) * fps);
    enc_pkt.dts = enc_pkt.pts;
    enc_pkt.duration = (video_st->time_base.den) / ((video_st->time_base.num) * fps);
    enc_pkt.pos = -1;


    ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
    if (ret != 0) {
        LOGE("av_interleaved_write_frame failed");
    }
    count++;
    av_packet_unref(&enc_pkt);
    av_frame_free(&pFrameYUV);
    av_free(buffers);
    (*jniEnv)->ReleaseByteArrayElements(jniEnv, yArray, yin, 0);
    (*jniEnv)->ReleaseByteArrayElements(jniEnv, uArray, uin, 0);
    (*jniEnv)->ReleaseByteArrayElements(jniEnv, vArray, vin, 0);

    return 0;
}


/**
 * 释放资源
 */
JNIEXPORT jint JNICALL Java_com_david_camerapush_ffmpeg_FFmpegHandler_close
        (JNIEnv *jniEnv, jobject instance) {
    if (video_st)
        avcodec_close(pCodecCtx);
    if (ofmt_ctx) {
        avio_close(ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
        ofmt_ctx = NULL;
    }
    return 0;
}

