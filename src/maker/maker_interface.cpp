#include "maker_interface.h"

#include "maker_def.h"
#include "maker.h"
#include "maker_io.h"



extern "C" {

#include <libavutil/samplefmt.h>

} // end extern "C"





/*******************************************************************************
create_maker_io
********************************************************************************/
MakerInterface*     create_maker_io()
{
    return  new MakerIO;
}





/*******************************************************************************
maker_encode_example
********************************************************************************/
void    maker_encode_example()
{
    EncodeSetting   setting;
    setting.io_type =   IO_Type::DEFAULT;
    //setting.io_type =   IO_Type::FILE_IO;
    //setting.io_type =   IO_Type::SRT_IO;


    // rmvb 是 variable bitrate. 目前還無法使用
    setting.filename    =   "H:\\output.mkv";
    setting.extension   =   "matroska";
    //setting.filename    =   "H:\\output.mp4";
    //setting.extension   =   "mp4";
    //setting.filename    =   "H:\\test.avi"; 
    //setting.extension   =   "avi";
    //setting.srt_port        =   "1234";
    setting.has_subtitle    =   true;

    VideoEncodeSetting  v_setting;
    v_setting.load_jpg_root_path    =   "J:\\jpg";
    v_setting.code_id   =   AV_CODEC_ID_H264;
    //v_setting.code_id   =   AV_CODEC_ID_H265;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG1VIDEO;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG2VIDEO;

    v_setting.width     =   1280;
    v_setting.height    =   720;

    v_setting.time_base.num     =   1001;
    v_setting.time_base.den     =   24000;

    /*
        b frame not support on rm
    */
    v_setting.gop_size      =   30;
    v_setting.max_b_frames  =   15;
    //v_setting.gop_size      =   10;
    //v_setting.max_b_frames  =   0;


    v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P12LE;

    v_setting.src_width     =   1280;
    v_setting.src_height    =   720;
    //v_setting.src_pix_fmt   =   AV_PIX_FMT_BGRA;    // for QImage
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGR24;    // for openCV


    AudioEncodeSetting  a_setting;
    a_setting.load_pcm_path     =   "J:\\test.pcm";
    a_setting.code_id     =   AV_CODEC_ID_MP3;
    //a_setting.code_id       =   AV_CODEC_ID_AAC;
    //a_setting.code_id       =   AV_CODEC_ID_AC3;
    //a_setting.code_id     =   AV_CODEC_ID_MP2;
    //a_setting.code_id       =   AV_CODEC_ID_VORBIS;
    //a_setting.code_id       =   AV_CODEC_ID_FLAC;

    //a_setting.bit_rate          =   320000;
    a_setting.bit_rate          =   128000;
    a_setting.sample_rate       =   48000;
    a_setting.channel_layout    =   3; // AV_CH_LAYOUT_STEREO = 3;
    a_setting.sample_fmt        =   static_cast<int>(AV_SAMPLE_FMT_S16);

    SubEncodeSetting   s_setting;
    s_setting.code_id       =   AV_CODEC_ID_ASS;
    //s_setting.code_id       =   AV_CODEC_ID_SUBRIP;
    //s_setting.code_id       =   AV_CODEC_ID_MOV_TEXT;
    s_setting.subtitle_file =   "J:\\test.ass";
    s_setting.subtitle_ext  =   "ass";


    Maker   maker;

    maker.init( setting, v_setting, a_setting, s_setting );
    maker.work();
    //maker.work_live_stream();
    maker.end();
}







/*******************************************************************************
output_by_io
********************************************************************************/
void    output_by_io( MediaInfo media_info, std::string _port, MakerInterface* maker )
{
    MakerIO*    maker_io    =   dynamic_cast<MakerIO*>(maker);
    if( maker_io == nullptr );
    {
        MYLOG( LOG::ERROR, "maker io convert fail." );
        return;
    }

    EncodeSetting   setting;
    setting.io_type         =   IO_Type::SRT_IO;    
    setting.srt_port        =   _port;
    setting.has_subtitle    =   false;

    VideoEncodeSetting  v_setting;
    v_setting.code_id   =   AV_CODEC_ID_H264;
    //v_setting.code_id   =   AV_CODEC_ID_H265;

    v_setting.width     =   media_info.width;
    v_setting.height    =   media_info.height;
    v_setting.pix_fmt   =   static_cast<AVPixelFormat>(media_info.pix_fmt);

    v_setting.time_base.num     =   media_info.time_num;
    v_setting.time_base.den     =   media_info.time_den;

    v_setting.gop_size      =   15;
    v_setting.max_b_frames  =   5;

    v_setting.src_width     =   media_info.width;
    v_setting.src_height    =   media_info.height;
    v_setting.src_pix_fmt   =   static_cast<AVPixelFormat>(media_info.pix_fmt);

    AudioEncodeSetting  a_setting;
    a_setting.code_id           =   AV_CODEC_ID_AAC;
    a_setting.bit_rate          =   128000;
    a_setting.sample_rate       =   media_info.sample_rate;
    a_setting.channel_layout    =   media_info.channel_layout;
    a_setting.sample_fmt        =   media_info.sample_fmt;  

    maker_io->init( setting, v_setting, a_setting );
    maker_io->work();
    maker_io->end();
}


