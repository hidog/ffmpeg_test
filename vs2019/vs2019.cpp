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


extern "C" {
#include "../ffmpeg_example/demuxing_decoding.h"
#include "../ffmpeg_example/ffplay.h"
#include <libavcodec/avcodec.h>

}

#include "../ffmpeg_example/muxing.h"


#include "maker/audio_encode.h"
#include "maker/video_encode.h"
#include "maker/maker.h"
#include "maker/mux.h"



int main()
{
#if 0

    muxing();
#elif 0

    Maker maker;
    maker.init();
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
