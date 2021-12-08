#include <iostream>

#include "player.h"


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


extern "C" {
#include "../ffmpeg_example/demuxing_decoding.h"
#include "../ffmpeg_example/ffplay.h"
}


int main()
{
#if 1
    char str[100] = "D:\\code\\test2.mkv";

    char **argv = new char* [10];
    argv[1] = str;

#if 0
    char str2[100] = "-vf";
    argv[1] = str2;
    char str3[100] = "subtitles='../../test.ssa':stream_index=0";
    argv[2] = str3;
#endif

    ffplay_main( 2, argv );
    //play_demuxing_decodeing();
#else

    Player  player;  

    player.set_input_file("D:\\code\\test.rmvb");
    player.init();
    player.play();
    player.end();
#endif

    return 0;
}
