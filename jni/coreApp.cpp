#include <jni.h>
#include <stdio.h>
#include "android_log.h"
#include "pthread.h"
#include "WlListener.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>

#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG  "OpenCV"

pthread_t callbackThread;

void *callBackT(void *data)
{
    //获取WlListener指针
    WlListener *wlListener = (WlListener *) data;
    //在子线程中调用回调方法
    wlListener->onCallBack(1, 200, "Child thread running success!");
    pthread_exit(&callbackThread);
}

JavaVM* jvm;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm,void* reserved){
    JNIEnv *env;
    jvm = vm;
    if(vm->GetEnv((void**)&env,JNI_VERSION_1_6)!=JNI_OK){
        return -1;
    }
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jint JNICALL Java_com_ffmpeg_FFmpegUtil_open(JNIEnv *env,
        jclass obj, jstring url_,jint duration) {
			
	char url[500]={0};
    sprintf(url, "%s", env->GetStringUTFChars(url_, NULL));
	LOGD("play video url->%s",url);
	
	 // 视频流的格式内容
    AVFormatContext *pFormatCtx = NULL;
	
	// Register all codecs and formats so that they can be used.
	av_register_all();
    avformat_network_init();
   
    // 读取文件的头部并且把信息保存到我们给的AVFormatContext结构
    if (avformat_open_input(&pFormatCtx, url, NULL, NULL) != 0)
    {
        LOGE("打开文件失败");
        return -1;
    }
    
    // 检查在文件中的流的信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        LOGE("文件中没有流的信息");
        avformat_close_input(&pFormatCtx);
        return -1;
    }
    
    // 打印有关输入或输出格式的详细信息
    av_dump_format(pFormatCtx, 0, url, 0);
    
    int videoStream = -1;
    // 找到第一个视频流
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1)
    {
        LOGE("没有找到第一个视频流");
        avformat_close_input(&pFormatCtx);
        return -1;
    }
    
    // 流中关于编解码器的信息就是被我们叫做"codec context"（编解码器上下文）的东西。
    // 这里面包含了流中所使用的关于编解码器的所有信息，现在我们有了一个指向他的指针。
    // 但是我们必需要找到真正的编解码器并且打开它
    // 创建编码器上下文
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
    if (pCodecCtx == NULL)
    {
        LOGE("创建编码器上下文失败");
        avformat_close_input(&pFormatCtx);
        return -1;
    }
    
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;
/*	
    if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codec) < 0)
    {
        LOGE("AVCodecContext 赋值失败");
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return -1;
    }*/
    
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        LOGE("没找到解码器");
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return -1;
    }
    
    // 打开avcodec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        LOGE("解码器打开失败");
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        return -1;
    }
    
    // YUV帧, 原始帧
    AVFrame *pFrame = av_frame_alloc();
    
    // 因为我们准备输出保存24位RGB色的PPM文件，我们必需把帧的格式从原来的转换为RGB。
    // FFMPEG将为我们做这些转换。
    // 在大多数项目中（包括我们的这个）我们都想把原始的帧转换成一个特定的格式。
    // 让我们先为转换来申请一帧的内存
    AVFrame *pFrameRGB = av_frame_alloc();
    
    // 即使我们申请了一帧的内存，当转换的时候，我们仍然需要一个地方来放置原始的数据。
    // 我们使用avpicture_get_size来获得我们需要的大小，然后手工申请内存空间：
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    
    // 现在我们使用avpicture_fill来把帧和我们新申请的内存来结合。
    // 关于AVPicture的结成：AVPicture结构体是AVFrame结构体的子集
    // ――AVFrame结构体的开始部分与AVPicture结构体是一样的。
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
    
    if (pFormatCtx->duration > duration)
    {
        // 播放指定duration的任意帧
        av_seek_frame(pFormatCtx, -1, duration * AV_TIME_BASE, AVSEEK_FLAG_ANY);
        // 清空解码器的缓存
        avcodec_flush_buffers(pCodecCtx);
    }
    
    if (pCodecCtx->pix_fmt == -1)
    {
        LOGE("获取视频像素失败");
        avformat_close_input(&pFormatCtx);
        return -1;
    }
    // 视频像素上下文
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_POINT, NULL, NULL, NULL);
    
    // 最后，我们已经准备好来从流中读取数据了。
    // 读取数据
    // 我们将要做的是通过读取包来读取整个视频流，
    // 然后把它解码成帧，最好后转换格式并且保存。
    AVPacket packet;
    int i, frameFinished;
    i = 0;
	
	WlListener *wlListener = new WlListener(jvm, env, env->NewGlobalRef(obj));
	
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            
            // Did we get a video frame?
            if (frameFinished) {
                // Convert the image from its native format to RGB.
                sws_scale(sws_ctx, (uint8_t const * const *) pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
                
                // Save the frame to disk.
                if (1) {
					LOGD("DATA-->");
					wlListener->onCallBack(0, 100, "JNIENV thread running success!");
					//开启子线程，并把WlListener指针传递到子线程中
					pthread_create(&callbackThread, NULL, callBackT, wlListener);
                 
                }
            }
        }
        
        // Free the packet that was allocated by av_read_frame.
        //av_free_packet(&packet); // Deprecated.
        av_packet_unref(&packet);
    }
    sws_freeContext(sws_ctx);
    
    // Free the RGB image.
    av_free(buffer);
    av_frame_free(&pFrameRGB);
    
    // Free the YUV frame.
    av_frame_free(&pFrame);
    
    avcodec_free_context(&pCodecCtx);
    
    // Close the codecs.
    avcodec_close(pCodecCtx);
    
    // Close the video file.
    avformat_close_input(&pFormatCtx);
	
	return 0;
}
