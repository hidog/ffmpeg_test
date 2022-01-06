#include "sub_encode.h"

#include "tool.h"


extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

} // end extern "C"




/*******************************************************************************
SubEncode::SubEncode()
********************************************************************************/
SubEncode::SubEncode()
    :   Encode()
{}





/*******************************************************************************
SubEncode::~SubEncode()
********************************************************************************/
SubEncode::~SubEncode()
{}







/*******************************************************************************
SubEncode::copy_sub_header()
********************************************************************************/
void    SubEncode::copy_sub_header()
{
    if( dec->subtitle_header != nullptr )
    {
        // if not set header, open subtitle enc will fail.
        ctx->subtitle_header        =   (uint8_t*)av_mallocz( dec->subtitle_header_size + 1 );   // 沒查到 + 1 的理由. 也有沒+1的. 都可執行
        memcpy( ctx->subtitle_header, dec->subtitle_header, dec->subtitle_header_size );
        ctx->subtitle_header_size   =   dec->subtitle_header_size;    
        //MYLOG( LOG::INFO, "subtitle header = \n%s", ctx->subtitle_header );    
    }
}




/*******************************************************************************
SubEncode::end()
********************************************************************************/
void    SubEncode::end()
{
    if( dec != nullptr )
    {
        avcodec_free_context( &dec );
        dec     =   nullptr;
    }

    sub_stream  =   nullptr;
    sub_idx     =   -1;
    last_pts    =   0;

    if( subtitle != nullptr )
    {
        av_free(subtitle);
        subtitle    =   nullptr;
    }

    if( sub_pkt != nullptr )
    {
        av_packet_free( &sub_pkt );
        sub_pkt =   nullptr;
    }

    if( subtitle_out != nullptr )
    {
        av_free(subtitle_out);
        subtitle_out    =   nullptr;
    }

    // note: header是自己額外allocate出來的,所以要在Encode::end()之前釋放
    if( ctx != nullptr )
    {
        if( ctx->subtitle_header    !=  nullptr )
            av_free( ctx->subtitle_header );
    }

    Encode::end();

    // close subtitle input
    if( fmt_ctx != nullptr )
    {
        avformat_close_input( &fmt_ctx );
        fmt_ctx     =   nullptr;
    }
}






/*******************************************************************************
SubEncode::get_queue_size()
********************************************************************************/
int     SubEncode::get_queue_size()
{
    return  sub_queue.size();
}




/*******************************************************************************
SubEncode::init()
********************************************************************************/
void    SubEncode::init( int st_idx, SubEncodeSetting setting, bool need_global_header )
{
    // note: 一些資料 subtitle encode 用不到, 但方便程式撰寫, 還是初始化那些資料.
    Encode::init( st_idx, setting.code_id );

    int ret =   0;

    //
    last_pts    =   0;
    sub_pkt     =   av_packet_alloc();
    if( sub_pkt == nullptr )
        MYLOG( LOG::ERROR, "alloc sub pkt fail." );

    subtitle    =   static_cast<AVSubtitle*>(av_malloc(sizeof(subtitle)));
    if( subtitle == nullptr )
        MYLOG( LOG::ERROR, "subtitle is null." );

    if( subtitle_out == nullptr )
    {
        subtitle_out    =   (uint8_t*)av_mallocz(subtitle_out_max_size);
        if( subtitle_out == nullptr )
            MYLOG( LOG::ERROR, "subtitle_out is null." );
    }


    // open subtitle file.    
    ret     =   open_subtitle_source( setting.subtitle_file );
    if( ret == ERROR )
        MYLOG( LOG::ERROR, "open subtitle source fail." );

    // ost->st->time_base = c->time_base;
    //
    ctx->pkt_timebase   =   AVRational{ 1, 1000 };   // 研究一下這邊該怎麼設置. 有程式碼說不影響結果, 實驗看看.
    //ctx->pkt_timebase   =   AVRational{ 1, AV_TIME_BASE };
    ctx->time_base      =   AVRational{ 1, 1000 };   
    //ctx->time_base      =   AVRational{ 1, 1 };


    copy_sub_header();

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

}






/*******************************************************************************
SubEncode::get_src_stream_timebase()
********************************************************************************/
AVRational  SubEncode::get_src_stream_timebase()
{
    return  sub_stream->time_base;
}






/*******************************************************************************
SubEncode::load_all_subtitle()

某些字幕檔不會按照時間來排序字幕資料,所以一次讀取完畢並且根據時間做重新排序.
會有字幕檔長度限制.
如果需要,再寫無長度限制的版本.
********************************************************************************/
void    SubEncode::load_all_subtitle()
{
    int     ret     =   0;

    while( true )
    {
        ret     =   av_read_frame( fmt_ctx, sub_pkt );
        if( ret < 0 )
            break;

        //MYLOG( LOG::DEBUG, "sub pkt pts = %lld", sub_pkt.pts );
        sub_queue.emplace( *sub_pkt );
    }
    MYLOG( LOG::INFO, "load subtitle file finish." );

#if 0
    // print queue for test.
    while( false == sub_queue.empty()) 
    {
        MYLOG( LOG::DEBUG, "pts = %lld, str = %s", sub_queue.top().pts, sub_queue.top().data );
        sub_queue.pop();
    }
#endif
}





/*******************************************************************************
SubEncode::open_subtitle_source()
********************************************************************************/
int     SubEncode::open_subtitle_source( std::string src_sub_file )
{
    int     ret     =   0;

    fmt_ctx     =   avformat_alloc_context();
    if( fmt_ctx == nullptr )
    {
        MYLOG( LOG::ERROR, "alloc fmt ctx fail." );
        return  ERROR;
    }

    ret     =   avformat_open_input( &fmt_ctx, src_sub_file.c_str(), nullptr, nullptr );
    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "oepn input %s fail.", src_sub_file.c_str() );
        return  ERROR;
    }

    ret     =   avformat_find_stream_info( fmt_ctx, nullptr );
    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "find stream info fail." );
        return  ERROR;
    }

    sub_idx     =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0 );
    if( sub_idx < 0 )
    {
        MYLOG( LOG::ERROR, "stream idx = %d", sub_idx );
        return  ERROR;
    }
    
    sub_stream  =   fmt_ctx->streams[sub_idx];
    if( sub_stream == nullptr )
    {
        MYLOG( LOG::ERROR, "sub stream is null." );
        return  ERROR;
    }

    MYLOG( LOG::INFO, "source subtitle file is %s", avcodec_get_name(sub_stream->codecpar->codec_id) );
    const AVCodec   *src_codec  =   avcodec_find_decoder( sub_stream->codecpar->codec_id );
    if( src_codec == nullptr )
    {
        MYLOG( LOG::ERROR, "src_codec is null." );
        return  ERROR;
    }

    dec     =   avcodec_alloc_context3( src_codec );
    if( dec == nullptr )
    {
        MYLOG( LOG::ERROR, "dec is null." );
        return  ERROR;
    }

    ret     =   avcodec_parameters_to_context( dec, sub_stream->codecpar );
    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "copy param fail." );
        return  ERROR;
    }

    //
    dec->pkt_timebase   =   sub_stream->time_base;

    // AV_TIME_BASE
    //uint8_t *subtitle_header;
    //int subtitle_header_size;
    //c->pkt_timebase = AVRational{ 1, 1000 }; // 看敘述這邊不影響結果, 驗證一下結合影片的時候是否需要修改
    ////c->pkt_timebase = AVRational{ 1, AV_TIME_BASE }; // 看敘述這邊不影響結果, 驗證一下結合影片的時候是否需要修改
    //c->time_base = AVRational{ 1, 1000 };
    //ost->st->time_base = c->time_base;


    ret     =   avcodec_open2( dec, src_codec, nullptr );
    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "avcodec_open2 fail");
        return  ERROR;
    }

    return  SUCCESS;
}




/*******************************************************************************
SubEncode::get_pts()
********************************************************************************/
int64_t     SubEncode::get_pts()
{
    if( sub_queue.empty() == true )
    {
        MYLOG( LOG::ERROR, "queue is empty." );
        return  0;
    }

    return  sub_queue.top().pts;
}





/*******************************************************************************
SubEncode::get_frame()
********************************************************************************/
AVFrame*    SubEncode::get_frame()
{
    assert(false);
    return  nullptr;
}






/*******************************************************************************
SubEncode::get_duration()
********************************************************************************/
int64_t     SubEncode::get_duration()
{
    if( subtitle == nullptr )
        MYLOG( LOG::ERROR, "subtitle is null." );
    return  subtitle->end_display_time - subtitle->start_display_time;
}






/*******************************************************************************
SubEncode::encode_subtitle()
********************************************************************************/
void    SubEncode::encode_subtitle()
{
    if( sub_queue.empty() == true )
        MYLOG( LOG::ERROR, "queue is empty." );

    int     ret     =   0, 
            got_sub =   0;

    *sub_pkt    =   sub_queue.top();
    sub_queue.pop();

    if( sub_pkt->size == 0 )
        MYLOG( LOG::ERROR, "sub pkt size is 0" );

    //MYLOG( LOG::DEBUG, "sub_pkt = %s", sub_pkt.data );
    //memset( subtitle, 0, sizeof(subtitle) );            
    ret     =   avcodec_decode_subtitle2( dec, subtitle, &got_sub, sub_pkt ); 
    av_packet_unref( sub_pkt );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "decode fail." );    

    //MYLOG( LOG::DEBUG, "decode subtitle = %s", subtitle->rects[0]->ass );
    if( got_sub > 0 )
    {
        int subtitle_out_size   =   avcodec_encode_subtitle( ctx , subtitle_out, subtitle_out_max_size, subtitle );   
        if( subtitle_out_size == 0 )
            MYLOG( LOG::ERROR, "subtitle_out_size = 0" );
        
        pkt->data           =   subtitle_out;
        pkt->size           =   subtitle_out_size;
        pkt->stream_index   =   stream_index;
    }
}





/*******************************************************************************
SubEncode::set_last_pts()
********************************************************************************/
void    SubEncode::set_last_pts( int64_t _pts )
{
    last_pts    =   _pts;
}




/*******************************************************************************
SubEncode::generate_flush_pkt()
********************************************************************************/
int     SubEncode::generate_flush_pkt()
{
    pkt->data   =   nullptr;
    pkt->size   =   0;
    pkt->pts    =   last_pts;
    pkt->dts    =   last_pts;

    pkt->duration       =   0;
    pkt->stream_index   =   stream_index;

    return  SUCCESS;
}





/*******************************************************************************
SubEncode::unref_subtitle()
********************************************************************************/
void    SubEncode::unref_subtitle()
{
    avsubtitle_free(subtitle);
}




/*******************************************************************************
SubEncode::unref_pkt()
********************************************************************************/
void    SubEncode::unref_pkt()
{
    av_free( pkt->data );
    Encode::unref_pkt();
}



/*******************************************************************************
SubEncode::get_subtitle_pts()
********************************************************************************/
int64_t     SubEncode::get_subtitle_pts()
{
    if( subtitle == nullptr )
        MYLOG( LOG::ERROR, "subtitle is null." );
    return  subtitle->pts;
}

