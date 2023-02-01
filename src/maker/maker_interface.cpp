#include "maker_interface.h"

#include "maker_def.h"
#include "maker.h"



/*******************************************************************************
maker_encode_example
********************************************************************************/
void    maker_encode_example()
{
    EncodeSetting   setting;

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
    //a_setting.channel_layout    =   3; // AV_CH_LAYOUT_STEREO = 3;
    a_setting.channels          =   2;
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





