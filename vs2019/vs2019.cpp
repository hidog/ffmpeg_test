#include <iostream>

#include "player.h"


/*
http://dranger.com/ffmpeg/tutorial05.html
http://dranger.com/ffmpeg/tutorial01.html

https://cpp.hotexamples.com/examples/-/-/avsubtitle_free/cpp-avsubtitle_free-function-examples.html



https://www.twblogs.net/a/5ef2639e26bc8c4a8eb3c99e
https://github.com/wang-bin/QtAV/blob/master/src/subtitle/SubtitleProcessorFFmpeg.cpp




ass_encode_frame
show_subtitle


decoder_decode_frame   ****
sub2video_update       *

try_decode_frame
init_subtitles


decode_frame


http://underpop.online.fr/f/ffmpeg/help/subtitles.htm.gz



https://cloud.tencent.com/developer/article/1357993
調用 avformat_match_stream_specifier： 分離audio，video流訊息，判断当前流的类型

https://www.itread01.com/content/1546569392.html

*/

extern "C" {
#include "../ffmpeg_example/demuxing_decoding.h"
#include "../ffmpeg_example/ffplay.h"
}


int main()
{
#if 1
    char str[100] = "D:\\code\\fmp2.avi";

    char **argv = new char* [10];
    argv[1] = str;

    ffplay_main( 2, argv );
    //play_demuxing_decodeing();
#else

    Player  player;  

    player.init();
    player.play();
    player.end();
#endif

    return 0;
}

//http://blog.360converter.com/archives/2931  youtube