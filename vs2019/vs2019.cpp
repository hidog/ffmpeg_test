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
#if 1
    /*char **argv = new char* [10];

    char str1[100] = "H:\\test.mp3";
    argv[1] = str1;

    encode_audio( 2, argv );*/

    muxing();
#elif 1

    Maker maker;
    maker.init();
    maker.work();

#elif 0

    
    AVCodecID id_arr[5] = { AV_CODEC_ID_MP3, AV_CODEC_ID_MP2, AV_CODEC_ID_AAC, AV_CODEC_ID_AC3, AV_CODEC_ID_FLAC };

    //for( int i = 0; i < 4; i++ )
    int i = 2;
    
    AudioEncode a_encoder;
    a_encoder.list_sample_format( id_arr[i] );
    a_encoder.list_sample_rate( id_arr[i] );
    a_encoder.list_channel_layout( id_arr[i] );
    
    a_encoder.init( id_arr[i] );
    //a_encoder.work( id_arr[i] );
    //a_encoder.end();
    


    VideoEncode v_encoder;

    v_encoder.init();
    //v_encoder.work();
    //v_encoder.end();


    auto *v = v_encoder.ctx;
    auto *a = a_encoder.ctx;


    Mux muxer;

    muxer.v_get_next_pts = std::bind( &VideoEncode::get_next_pts, &v_encoder );
    muxer.a_get_next_pts = std::bind( &AudioEncode::get_next_pts, &a_encoder );

    muxer.v_get_frame = std::bind( &VideoEncode::get_frame, &v_encoder );
    muxer.a_get_frame = std::bind( &AudioEncode::get_frame, &a_encoder );

    muxer.v_send_frame = std::bind( &VideoEncode::send_frame, &v_encoder, std::placeholders::_1 );
    muxer.v_recv_frame = std::bind( &VideoEncode::recv_frame, &v_encoder );
    muxer.v_get_pkt = std::bind( &VideoEncode::get_pkt, &v_encoder );

    muxer.a_send_frame = std::bind( &AudioEncode::send_frame, &a_encoder, std::placeholders::_1 );
    muxer.a_recv_frame = std::bind( &AudioEncode::recv_frame, &a_encoder );
    muxer.a_get_pkt = std::bind( &AudioEncode::get_pkt, &a_encoder );

    muxer.init( v, a );
    muxer.work();



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
