#include "sub_encode.h"

#include "tool.h"


extern "C" {

#include <libavcodec/avcodec.h>

} // end extern "C"




/*******************************************************************************
SubEncode::SubEncode()
********************************************************************************/
SubEncode::SubEncode()
{}





/*******************************************************************************
SubEncode::~SubEncode()
********************************************************************************/
SubEncode::~SubEncode()
{}





/*******************************************************************************
SubEncode::init()
********************************************************************************/
void    SubEncode::init( int st_idx, SubEncodeSetting setting, bool need_global_header )
{
    // note: 一些資料 subtitle encode 用不到, 但方便程式撰寫, 還是初始化那些資料.
    Encode::init( st_idx, setting.code_id );

    int ret =   0;


    //ost->st->time_base = c->time_base;

    //
    ctx->pkt_timebase   =   AVRational{ 1, 1000 };   // 研究一下這邊該怎麼設置. 有程式碼說不影響結果, 實驗看看.
    //ctx->pkt_timebase   =   AVRational{ 1, AV_TIME_BASE };
    ctx->time_base      =   AVRational{ 1, 1000 };   

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

    // open subtitle file.
    ret     =   open_subtitle_source();
    if( ret < 0 )
        MYLOG( LOG::ERROR, "open subtitle source fail." );
}




/*******************************************************************************
SubEncode::open_subtitle_source()
********************************************************************************/
int     SubEncode::open_subtitle_source()
{

}




/*******************************************************************************
SubEncode::get_pts()
********************************************************************************/
int64_t     SubEncode::get_pts()
{
    return  0;
}





/*******************************************************************************
SubEncode::get_frame()
********************************************************************************/
AVFrame*    SubEncode::get_frame()
{
    return  0;
}