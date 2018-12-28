package com.david.camerapush.ffmpeg;

/**
 * @Author: liuwei
 * @Create: 2018/12/21 9:35
 * @Description:
 */
public class FFmpegHandler {
    private FFmpegHandler() {
    }

    private static class SingletonInstance {
        private static final FFmpegHandler INSTANCE = new FFmpegHandler();
    }

    public static FFmpegHandler getInstance() {
        return SingletonInstance.INSTANCE;
    }


    static {
        System.loadLibrary("ffmpeg-handler");
//        System.loadLibrary("avcodec-58");
//        System.loadLibrary("avdevice-58");
//        System.loadLibrary("avfilter-7");
//        System.loadLibrary("avformat-58");
//        System.loadLibrary("avutil-56");
//        System.loadLibrary("postproc-55");
//        System.loadLibrary("swresample-3");
//        System.loadLibrary("swscale-5");

    }

    public native int init(String outUrl);

    public native int pushCameraData(byte[] buffer,int ylen,byte[] ubuffer,int ulen,byte[] vbuffer,int vlen);

    public native int close();
}
