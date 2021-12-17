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

#include "maker/audio_encode.h"
#include "maker/video_encode.h"
#include "maker/maker.h"



int main()
{
#if 0
    /*char **argv = new char* [10];

    char str1[100] = "H:\\test.mp3";
    argv[1] = str1;

    encode_audio( 2, argv );*/
#elif 1


    AVCodecID id_arr[5] = { AV_CODEC_ID_MP3, AV_CODEC_ID_MP2, AV_CODEC_ID_AAC, AV_CODEC_ID_AC3, AV_CODEC_ID_FLAC };

    for( int i = 0; i < 4; i++ )
    //int i = 4;
    {
        AudioEncode a_encoder;
        a_encoder.list_sample_format( id_arr[i] );
        a_encoder.list_sample_rate( id_arr[i] );
        a_encoder.list_channel_layout( id_arr[i] );

        a_encoder.init( id_arr[i] );
        a_encoder.work( id_arr[i] );
        a_encoder.end();
    }


    VideoEncode v_encoder;

    v_encoder.init();
    v_encoder.work();
    v_encoder.end();

    
#else

    Player  player;  

    player.set_input_file("D:\\code\\test2.mkv");
    player.set_sub_file("D:\\code\\test2.mkv");

    player.init();
    player.play();
    player.end();
#endif

    return 0;
}
