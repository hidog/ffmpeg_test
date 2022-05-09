#include "translate.h"

#include <QString>
#include "player/player.h"
#include "maker/maker.h"




/*******************************************************************************
translate_media_file
********************************************************************************/
void    translate_media_file()
{
    std::string     input_file  =   "H:/1.mkv";
    std::string     audio_file  =   "H:/a.data";

    auto setting    =   extraction_audio( input_file, audio_file );

    std::string     output_file     =   "H:/output.mkv";
    VideoDecodeSetting  vs   =   std::get<0>(setting);
    AudioDecodeSetting  as   =   std::get<1>(setting);

    re_encode_media( output_file, vs, as );
}




/*******************************************************************************
extraction_audio
********************************************************************************/
std::tuple<VideoDecodeSetting, AudioDecodeSetting>   extraction_audio( std::string input_file, std::string audio_file )
{
#ifdef FFMPEG_TEST
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
#else
    return  std::tuple<VideoDecodeSetting, AudioDecodeSetting>();
#endif
}




/*******************************************************************************
extraction_audio
********************************************************************************/
void    re_encode_media( std::string output_file, VideoDecodeSetting vs, AudioDecodeSetting as )
{
#ifdef FFMPEG_TEST
    EncodeSetting   setting;
    setting.io_type =   IO_Type::DEFAULT;

    setting.filename        =   output_file;
    setting.extension       =   "matroska";
    setting.has_subtitle    =   false;

    VideoEncodeSetting  v_setting;
    v_setting.code_id   =   static_cast<AVCodecID>(vs.code_id);
    v_setting.width     =   vs.width;
    v_setting.height    =   vs.height;

    v_setting.time_base.num     =   vs.time_base_num;
    v_setting.time_base.den     =   vs.time_base_den;

    v_setting.gop_size      =   30;     // use for nvenc
    v_setting.max_b_frames  =   15;

    if( vs.pix_fmt == AV_PIX_FMT_YUV420P )
        v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    else
        v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;

    v_setting.src_width     =   vs.width;
    v_setting.src_height    =   vs.height;
    //v_setting.src_pix_fmt   =   AV_PIX_FMT_BGRA;    // for QImage
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGR24;    // for openCV


    AudioEncodeSetting  a_setting;
    a_setting.load_pcm_path     =   "H:\\a.pcm";
    a_setting.code_id           =   static_cast<AVCodecID>(as.code_id);
    a_setting.bit_rate          =   320000;
    //a_setting.bit_rate          =   128000;
    a_setting.sample_rate       =   as.sample_rate;
    a_setting.channel_layout    =   as.channel_layout;
    a_setting.sample_fmt        =   as.sample_type;

    SubEncodeSetting   s_setting;
    s_setting.code_id       =   AV_CODEC_ID_ASS;
    //s_setting.code_id       =   AV_CODEC_ID_SUBRIP;
    //s_setting.code_id       =   AV_CODEC_ID_MOV_TEXT;
    s_setting.subtitle_file =   "F:\\test.ass";
    s_setting.subtitle_ext  =   "ass";


    Maker   maker;

    maker.init( setting, v_setting, a_setting, s_setting );
    maker.work();
    maker.end();

#endif
}
