#include "translate.h"

#include <QString>
#include <thread>

#include "player/player.h"
#include "maker/maker.h"


namespace {
std::string     audio_file;
} // end anonymous namespace 



/*******************************************************************************
run_play
********************************************************************************/
void    run_play( std::string input_file )
{
    DecodeSetting   setting;
    setting.io_type     =   IO_Type::DEFAULT;
    setting.filename    =   input_file;    

    Player  player;  

    player.set( setting );
    player.init();
    player.remove_subtitle();
    
    player.play_decode_video();
    player.end();
}




/*******************************************************************************
translate_media_file
********************************************************************************/
void    translate_media_file()
{
    audio_file  =   "H:/a.data";

    std::string     input_file  =   "H:/1.mkv";
    std::string     sub_file    =   "H:/1.ass";

    auto setting    =   extraction_audio( input_file );

    std::thread     thr( run_play, input_file );

    std::string     output_file     =   "H:/output.mkv";
    VideoDecodeSetting  vs   =   std::get<0>(setting);
    AudioDecodeSetting  as   =   std::get<1>(setting);

    vs.gop_size      =   30;     // use for nvenc
    vs.max_b_frames  =   5;
    vs.cq            =   "25";

    re_encode_media( output_file, sub_file, vs, as );

    thr.join();
}




/*******************************************************************************
extraction_audio
********************************************************************************/
std::tuple<VideoDecodeSetting, AudioDecodeSetting>   extraction_audio( std::string input_file )
{
    DecodeSetting   setting;
    setting.io_type     =   IO_Type::DEFAULT;
    setting.filename    =   input_file;    

    VideoDecodeSetting  vs;
    AudioDecodeSetting  as;

    Player  player;  

    player.set( setting );
    player.init();

    vs   =   player.get_video_setting();
    as   =   player.get_audio_setting();

    player.set_output_audio_pcm_path( audio_file );

    player.play_output_audio();
    player.end();

    return  std::make_tuple( vs, as );
}




/*******************************************************************************
extraction_audio
********************************************************************************/
void    re_encode_media( std::string output_file, std::string sub_file, VideoDecodeSetting vs, AudioDecodeSetting as )
{
    EncodeSetting   setting;
    setting.io_type =   IO_Type::DEFAULT;

    setting.filename        =   output_file;
    setting.extension       =   "matroska";
    setting.has_subtitle    =   sub_file == "" ? false : true;

    VideoEncodeSetting  v_setting;
    v_setting.code_id   =   static_cast<AVCodecID>(vs.code_id);
    v_setting.width     =   vs.width;
    v_setting.height    =   vs.height;

    v_setting.time_base.num     =   vs.time_base_num;
    v_setting.time_base.den     =   vs.time_base_den;

    v_setting.gop_size      =   vs.gop_size;     // use for nvenc
    v_setting.max_b_frames  =   vs.max_b_frames;
    v_setting.cq            =   vs.cq;

    if( vs.pix_fmt == AV_PIX_FMT_YUV420P )
        v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    else
        v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;

    v_setting.src_width     =   vs.width;
    v_setting.src_height    =   vs.height;
    v_setting.src_pix_fmt   =   static_cast<AVPixelFormat>(vs.pix_fmt);

    AudioEncodeSetting  a_setting;
    a_setting.load_pcm_path     =   audio_file;
    a_setting.code_id           =   static_cast<AVCodecID>(as.code_id);
    a_setting.bit_rate          =   320000;
    //a_setting.bit_rate          =   128000;
    a_setting.sample_rate       =   as.sample_rate;
    a_setting.channel_layout    =   as.channel_layout;
    a_setting.sample_fmt        =   as.sample_fmt;

    SubEncodeSetting   s_setting;
    s_setting.code_id       =   AV_CODEC_ID_ASS;
    //s_setting.code_id       =   AV_CODEC_ID_SUBRIP;
    //s_setting.code_id       =   AV_CODEC_ID_MOV_TEXT;
    s_setting.subtitle_file =   sub_file;
    s_setting.subtitle_ext  =   "ass";


    Maker   maker;

    maker.init( setting, v_setting, a_setting, s_setting );
    maker.work_translate();
    maker.end();

}
