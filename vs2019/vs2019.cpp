﻿#include <iostream>

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
*/


#include "../ffmpeg_example/muxing.h"
#include "maker/maker.h"




int main()
{
#if 0

    muxing();

#elif 1

    EncodeSetting   setting;
    setting.filename    =   "J:\\test2.mkv";
    setting.extension   =   "matroska";
    //setting.filename    =   "J:\\test2.mp4";
    //setting.extension   =   "mp4";


    VideoEncodeSetting v_setting;
    v_setting.code_id   =   AV_CODEC_ID_H264;
    //v_setting.code_id   =   AV_CODEC_ID_H265;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG1VIDEO;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG2VIDEO;

    v_setting.width     =   720;
    v_setting.height    =   480;

    v_setting.time_base.num     =   1001;
    v_setting.time_base.den     =   24000;

    v_setting.gop_size      =   30;
    v_setting.max_b_frames  =   15;
    //v_setting.gop_size      =   200;  // h265不能設太大
    //v_setting.max_b_frames  =   150;

    v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;

    v_setting.src_width     =   1920;
    v_setting.src_height    =   1080;
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGRA;    // for QImage


    AudioEncodeSetting a_setting;
    //a_setting.code_id = AV_CODEC_ID_MP3;
    a_setting.code_id = AV_CODEC_ID_AAC;
    //a_setting.code_id       =   AV_CODEC_ID_AC3;
    a_setting.bit_rate      =   320000;
    a_setting.sample_rate   =   48000;




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
