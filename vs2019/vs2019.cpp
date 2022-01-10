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

https://libav.org/avconv.html#Subtitle-options_003a

https://en.wikibooks.org/wiki/FFMPEG_An_Intermediate_Guide/subtitle_options#Set_Subtitles_Character_Encoding_Conversion

https://www.itread01.com/content/1544555544.html







https://github.com/FFmpeg/FFmpeg/blob/master/libavfilter/vf_subtitles.c

static const AVOption subtitles_options[] = {
COMMON_OPTIONS
{"charenc",      "set input character encoding", OFFSET(charenc),      AV_OPT_TYPE_STRING, {.str = NULL}, 0, 0, FLAGS},
{"stream_index", "set stream index",             OFFSET(stream_index), AV_OPT_TYPE_INT,    { .i64 = -1 }, -1,       INT_MAX,  FLAGS},
{"si",           "set stream index",             OFFSET(stream_index), AV_OPT_TYPE_INT,    { .i64 = -1 }, -1,       INT_MAX,  FLAGS},
{"force_style",  "force subtitle style",         OFFSET(force_style),  AV_OPT_TYPE_STRING, {.str = NULL}, 0, 0, FLAGS},
{NULL},
};



-metadata:s:s:1 language=English 
-metadata:s:s:2 language=Dansk

ffmpeg -i input.mkv -map 0 -c copy -c:s mov_text -metadata:s:s:0 language=eng -metadata:s:s:1 language=ipk output.mp4


https://srtlab.github.io/srt-cookbook/apps/ffmpeg/
ffmpeg -f lavfi -re -i smptebars=duration=60:size=1280x720:rate=30 -f lavfi -re \
-i sine=frequency=1000:duration=60:sample_rate=44100 -pix_fmt yuv420p \
-c:v libx264 -b:v 1000k -g 30 -keyint_min 120 -profile:v baseline \
-preset veryfast -f mpegts "srt://127.0.0.1:4200?pkt_size=1316"




底下這個命令可以保持font size. 但需要研究
ffmpeg -i abc.mkv -i abc.srt -map 0:0 -map 0:1 -map 1:0 -c:s copy output.mkv



ffmpeg -h ass




https://www.jianshu.com/p/f33910818a1c



https://github.com/nldzsz/ffmpeg-demo/blob/master/cppsrc/Subtitles.cpp



https://gist.github.com/TBNolan/a887c5d069425119dd41461b779aa75b



https://blog.csdn.net/qq_21743659/article/details/109305411


http://www.famous1993.com.tw/tech/tech545.html


https://codertw.com/%E7%A8%8B%E5%BC%8F%E8%AA%9E%E8%A8%80/435707/
*/


/*
失敗
ffmpeg -i mysubs.srt -filter "subtitles=mysubs.srt:force_style='Fontname=Helvetica,Fontsize=16,PrimaryColour=&H23EEF1&,BackColour=&H000000&,BorderStyle=3,Outline=3,Shadow=0,Alignment=2,MarginL=10,MarginR=10,MarginV=10,Encoding=0'" mysubs.ass

ffmpeg -i mysubs.srt -filter 
"subtitles=mysubs.srt:force_style='
Fontname=Helvetica,
Fontsize=16,
PrimaryColour=&H23EEF1&,
BackColour=&H000000&,
BorderStyle=3,
Outline=3,S
hadow=0,
Alignment=2,
MarginL=10,
MarginR=10,
MarginV=10,
Encoding=0'" 
mysubs.ass
*/



#include "maker/maker.h"





int main()
{

    maker_encode_example();

    //player_decode_example();

    return 0;
}



