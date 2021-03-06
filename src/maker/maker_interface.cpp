#include "maker_interface.h"
#include "maker_io.h"






/*******************************************************************************
create_maker_io
********************************************************************************/
MakerInterface*     create_maker_io()
{
    MakerInterface*     ptr     =   new MakerIO;
    return  ptr;
}





/*******************************************************************************
maker_encode_example
********************************************************************************/
void    maker_encode_example()
{
    EncodeSetting   setting;
    setting.io_type =   IO_Type::DEFAULT;

    // rmvb 是 variable bitrate. 目前還無法使用
    setting.filename    =   "F:\\output.mkv";
    setting.extension   =   "matroska";
    //setting.filename    =   "H:\\output.mp4";
    //setting.extension   =   "mp4";
    //setting.filename    =   "H:\\test.avi"; 
    //setting.extension   =   "avi";
    setting.has_subtitle    =   false;

    VideoEncodeSetting  v_setting;
    v_setting.load_jpg_root_path    =   "F:\\jpg";
    //v_setting.code_id   =   AV_CODEC_ID_H264;
    v_setting.code_id   =   AV_CODEC_ID_H265;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG1VIDEO;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG2VIDEO;

    v_setting.width     =   1920;
    v_setting.height    =   1080;

    v_setting.time_base.num     =   1001;
    v_setting.time_base.den     =   24000;

    /*
        b frame not support on rm
    */
    //v_setting.gop_size      =   30;
    //v_setting.max_b_frames  =   15;
    v_setting.gop_size      =   15;     // use for nvenc
    v_setting.max_b_frames  =   0;


    v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P12LE;

    v_setting.src_width     =   1920;
    v_setting.src_height    =   1080;
    //v_setting.src_pix_fmt   =   AV_PIX_FMT_BGRA;    // for QImage
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGR24;    // for openCV


    AudioEncodeSetting  a_setting;
    a_setting.load_pcm_path     =   "F:\\test.pcm";
    //a_setting.code_id     =   AV_CODEC_ID_MP3;
    //a_setting.code_id       =   AV_CODEC_ID_AAC;
    //a_setting.code_id       =   AV_CODEC_ID_AC3;
    //a_setting.code_id     =   AV_CODEC_ID_MP2;
    //a_setting.code_id       =   AV_CODEC_ID_VORBIS;
    a_setting.code_id       =   AV_CODEC_ID_FLAC;

    a_setting.bit_rate          =   320000;
    //a_setting.bit_rate          =   128000;
    a_setting.sample_rate       =   48000;
    a_setting.channel_layout    =   3; // AV_CH_LAYOUT_STEREO = 3;
    a_setting.sample_fmt        =   static_cast<int>(AV_SAMPLE_FMT_S16);

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
}







/*******************************************************************************
output_by_io
********************************************************************************/
void    output_by_io( MediaInfo media_info, std::string _port, MakerInterface* maker )
{
    MakerIO*    maker_io    =   dynamic_cast<MakerIO*>(maker);
    if( maker_io == nullptr )
    {
        MYLOG( LOG::L_ERROR, "maker io convert fail." );
        return;
    }

    EncodeSetting   setting;
    setting.io_type         =   IO_Type::SRT_IO;    
    setting.srt_port        =   _port;
    setting.has_subtitle    =   false;

    VideoEncodeSetting  v_setting;
    //v_setting.code_id   =   AV_CODEC_ID_H264;
    v_setting.code_id   =   AV_CODEC_ID_H265;

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


