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
*/



#include "maker/maker.h"




int main()
{
#if 0

    muxing();
#elif 1

    VideoEncodeSetting v_setting;
    //v_setting.code_id   =   AV_CODEC_ID_H264;
    //v_setting.code_id   =   AV_CODEC_ID_H265;
    v_setting.code_id   =   AV_CODEC_ID_MPEG1VIDEO;
    v_setting.width     =   1920;
    v_setting.height    =   1080;

    AudioEncodeSetting a_setting;
    //a_setting.code_id = AV_CODEC_ID_MP3;
    //a_setting.code_id = AV_CODEC_ID_AAC;
    a_setting.code_id = AV_CODEC_ID_AC3;
    a_setting.bit_rate = 320000;
    a_setting.sample_rate = 48000;

    Maker maker;

    maker.init( v_setting, a_setting );
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
