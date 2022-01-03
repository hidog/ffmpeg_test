#include "subtitle_encode.h"



/*******************************************************************************
SubtitleEncode::SubtitleEncode()
********************************************************************************/
SubtitleEncode::SubtitleEncode()
{}





/*******************************************************************************
SubtitleEncode::~SubtitleEncode()
********************************************************************************/
SubtitleEncode::~SubtitleEncode()
{}





/*******************************************************************************
SubtitleEncode::init()
********************************************************************************/
void    SubtitleEncode::init( int st_idx, SubtitleEncodeSetting setting, bool need_global_header )
{
    // note: ㄇ戈 subtitle encode ノぃ, よ獽祘Α级糶, 临琌﹍てêㄇ戈.
    Encode::init( st_idx, setting.code_id );

    int ret =   0;

    //
    ctx->bit_rate   =   3000000;
    /*ret = av_opt_set_int( ctx->priv_data, "crf", 50, 0 );
    if( ret < 0 )
    MYLOG( LOG::ERROR, "error");*/

    ctx->width      =   setting.width;
    ctx->height     =   setting.height;

    ctx->time_base  =   setting.time_base; 
    ctx->framerate.num  =   setting.time_base.den; 
    ctx->framerate.den  =   setting.time_base.num;

    ctx->gop_size       =   setting.gop_size;
    ctx->max_b_frames   =   setting.max_b_frames;

    ctx->pix_fmt        =   setting.pix_fmt;

    // ┏把计ゼ秨场砞竚,ぇσ璶ぃ璶秨
    ctx->me_subpel_quality  =   10;
    if( codec->id == AV_CODEC_ID_H264 || codec->id == AV_CODEC_ID_H265 )
        av_opt_set( ctx->priv_data, "preset", "medium", 0);

    if( ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO )
        ctx->mb_decision    =   FF_MB_DECISION_RD;

    // need before avcodec_open2
    if( need_global_header == true )
        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // open codec.
    //AVDictionary *opt_arg = nullptr;
    //AVDictionary *opt = NULL;
    //av_dict_copy(&opt, opt_arg, 0);
    //ret     =   avcodec_open2( ctx, codec, &opt );

    ret     =   avcodec_open2( ctx, codec, nullptr );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "open fail" );

    // frame setting
    frame->format   =   ctx->pix_fmt;
    frame->width    =   ctx->width;
    frame->height   =   ctx->height;

    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "get buffer fail." );

    //
    init_sws( setting );
}




/*******************************************************************************
SubtitleEncode::get_pts()
********************************************************************************/
int64_t     SubtitleEncode::get_pts()
{
    return  0;
}





/*******************************************************************************
SubtitleEncode::get_frame()
********************************************************************************/
AVFrame*    SubtitleEncode::get_frame()
{
    return  0;
}