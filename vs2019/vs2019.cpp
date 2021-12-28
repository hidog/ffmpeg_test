#include <iostream>

#include "player/player.h"


/*
https://github.com/leandromoreira/ffmpeg-libav-tutorial     教學
https://kkc.github.io/2019/01/12/ffmpeg-libav-decode-note/

http://dranger.com/ffmpeg/tutorial05.html  // 教學
http://dranger.com/ffmpeg/tutorial01.html

https://github.com/wang-bin/QtAV   // 其他人用QT寫的播放器

https://cloud.tencent.com/developer/article/1357993  // 可參考的教學
https://www.itread01.com/content/1546569392.html     // ffmpeg 參數

https://www.programmersought.com/article/85494009767/   ffmpeg教學
https://github.com/mengps/FFmpeg-Learn

https://www.cnblogs.com/linuxAndMcu/p/14706012.html

https://www.jianshu.com/p/fd43c1c82945
https://www.jianshu.com/p/bf5e54f553a4

https://www.itread01.com/content/1545479823.html
*/


#include "../ffmpeg_example/muxing.h"
#include "maker/maker.h"


extern "C" {
#include "../ffmpeg_example/resampling_audio.h"
}




int main()
{
#if 0
    //test_aac();

    //resample_audio();
    //muxing();

    AudioEncode ae;
    ae.list_sample_format(AV_CODEC_ID_VORBIS);

#elif 1

    EncodeSetting   setting;
    //setting.filename    =   "J:\\test2.mkv";
    //setting.extension   =   "matroska";
    //setting.filename    =   "J:\\test2.mp4";
    //setting.extension   =   "mp4";
    setting.filename    =   "J:\\test2.mpeg";  // rmvb 是 variable bitrate. 目前還無法使用
    setting.extension   =   "mpeg";

    VideoEncodeSetting  v_setting;
    //v_setting.code_id   =   AV_CODEC_ID_H264;
    //v_setting.code_id   =   AV_CODEC_ID_H265;
    v_setting.code_id   =   AV_CODEC_ID_MPEG1VIDEO;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG2VIDEO;

    v_setting.width     =   1280;
    v_setting.height    =   720;

    v_setting.time_base.num     =   1001;
    v_setting.time_base.den     =   24000;

    /*
        b frame not support on rm
    */
    v_setting.gop_size      =   30;
    v_setting.max_b_frames  =   15; 
    //v_setting.gop_size      =   12;
    //v_setting.max_b_frames  =   0; 


    v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;

    v_setting.src_width     =   1920;
    v_setting.src_height    =   1080;
    //v_setting.src_pix_fmt   =   AV_PIX_FMT_BGRA;    // for QImage
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGR24;    // for openCV


    AudioEncodeSetting  a_setting;
    //a_setting.code_id     =   AV_CODEC_ID_MP3;
    //a_setting.code_id       =   AV_CODEC_ID_AAC;
    //a_setting.code_id       =   AV_CODEC_ID_AC3;
    a_setting.code_id     =   AV_CODEC_ID_MP2;
    //a_setting.code_id       =   AV_CODEC_ID_VORBIS;

    a_setting.bit_rate          =   320000;
    a_setting.sample_rate       =   48000;
    a_setting.channel_layout    =   3; // AV_CH_LAYOUT_STEREO = 3;



    Maker maker;

    maker.init( setting, v_setting, a_setting );
    maker.work();
    maker.end();

#else

    Player  player;  

    player.set_input_file("D:/code/test2.mkv");
    player.set_sub_file("D:/code/test2.mkv");

    player.init();
    player.play();
    player.end();
#endif

    return 0;
}
