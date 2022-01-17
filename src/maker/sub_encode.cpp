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
{
    end();
}







/*******************************************************************************
SubEncode::copy_sub_header()
********************************************************************************/
void    SubEncode::copy_sub_header()
{
    if( dec->subtitle_header != nullptr )
    {
        // if not set header, open subtitle enc will fail.
        ctx->subtitle_header        =   (uint8_t*)av_mallocz( dec->subtitle_header_size );   // 沒查到 + 1 的理由. 也有沒+1的. 都可執行
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
    while( sub_queue.empty() == false )
    {
        auto    pkt =   sub_queue.top();
        av_packet_unref(&pkt);
        sub_queue.pop();
    }

    got_sub     =   -1;
    sub_stream  =   nullptr;
    sub_idx     =   -1;
    last_pts    =   0;

    if( sub_pkt != nullptr )
    {
        av_packet_free( &sub_pkt );
        sub_pkt =   nullptr;
    }

    if( sub_buf != nullptr )
    {
        av_free(sub_buf);
        sub_buf    =   nullptr;
    }

    // note: header是自己額外allocate出來的,所以要在Encode::end之前釋放
    if( ctx != nullptr )
    {
        if( ctx->subtitle_header != nullptr )        
            av_free( ctx->subtitle_header );
        ctx->subtitle_header    =   nullptr;
    }

    Encode::end();

    // close subtitle input
    if( dec != nullptr )
    {
        avcodec_free_context( &dec );
        dec     =   nullptr;
    }

    if( fmt_ctx != nullptr )
    {
        avformat_close_input( &fmt_ctx );
        fmt_ctx     =   nullptr;
    }
}





/*******************************************************************************
SubEncode::send_frame()
********************************************************************************/
int     SubEncode::send_frame()
{
    return encode_subtitle();
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

    int     ret =   0;

    //
    got_sub     =   -1;
    last_pts    =   0;
    sub_pkt     =   av_packet_alloc();
    if( sub_pkt == nullptr )
        MYLOG( LOG::ERROR, "alloc sub pkt fail." );

    if( sub_buf == nullptr )
    {
        sub_buf    =   (uint8_t*)av_mallocz(sub_buf_max_size);
        if( sub_buf == nullptr )
            MYLOG( LOG::ERROR, "sub_buf is null." );
    }

    // open subtitle file.    
    ret     =   open_subtitle_source( setting.subtitle_file );
    if( ret == ERROR )
        MYLOG( LOG::ERROR, "open subtitle source fail." );

    // 需設置
    ctx->pkt_timebase   =   AVRational{ 1, 1000 }; 
    ctx->time_base      =   AVRational{ 1, 1000 };   

    copy_sub_header();

    // need before avcodec_open2
    if( need_global_header == true )
        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

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
SubEncode::get_duration()
********************************************************************************/
int64_t     SubEncode::get_duration()
{
    return  sub.end_display_time - sub.start_display_time;
}




/*******************************************************************************
SubEncode::recv_frame()
********************************************************************************/
int     SubEncode::recv_frame()
{
    /* 
        note: 理論上純文字字幕不像 video 那樣 send 一次, recv 多次.
        如果需要 recv 多次, 會需要改掉這邊的設計.
    */    
    if( got_sub > 0 )
    {
        got_sub     =   AVERROR(EAGAIN);
        return  1;
    }
    else
        return  got_sub;
}





/*******************************************************************************
SubEncode::encode_subtitle()
********************************************************************************/
int     SubEncode::encode_subtitle()
{
    if( sub_queue.empty() == true )
    {
        MYLOG( LOG::WARN, "queue is empty." );
        return  ERROR;
    }

    int     ret     =   0;
    *sub_pkt    =   sub_queue.top();
    //sub_queue.pop();

    if( sub_pkt->size == 0 )
        MYLOG( LOG::ERROR, "sub pkt size is 0" );

    //MYLOG( LOG::DEBUG, "sub_pkt = %s", sub_pkt.data );
    //memset( subtitle, 0, sizeof(subtitle) );            
    ret     =   avcodec_decode_subtitle2( dec, &sub, &got_sub, sub_pkt ); 
    av_packet_unref( sub_pkt );
    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "decode fail." );
        return  ERROR;
    }

    //MYLOG( LOG::DEBUG, "decode subtitle = %s", subtitle->rects[0]->ass );
    if( got_sub > 0 )
    {
        int sub_buf_size   =   avcodec_encode_subtitle( ctx , sub_buf, sub_buf_max_size, &sub );   
        if( sub_buf_size == 0 )
            MYLOG( LOG::WARN, "subtitle_out_size = 0" );
        
        pkt->data           =   sub_buf;
        pkt->size           =   sub_buf_size;
        pkt->stream_index   =   stream_index;

        return  HAVE_FRAME;
    }
    else
        return  SUCCESS;
}






/*******************************************************************************
SubEncode::next_frame()
********************************************************************************/
void    SubEncode::next_frame()  
{
    assert( sub_queue.empty() == false );
    sub_queue.pop();
    if( sub_queue.empty() == true )
        eof_flag    =   true;
} 






/*******************************************************************************
SubEncode::encode_timestamp()
********************************************************************************/
void    SubEncode::encode_timestamp()
{
    if( pkt == nullptr )
        MYLOG( LOG::ERROR, "pkt is null." );

    AVRational  ctx_tb          =   get_timebase();
    AVRational  stb             =   get_stream_time_base();
    int64_t     subtitle_pts    =   get_subtitle_pts();
    int64_t     duration        =   get_duration();    

    pkt->pts         =   av_rescale_q( subtitle_pts, AVRational{1,AV_TIME_BASE}, stb );
    pkt->duration    =   av_rescale_q( duration, ctx_tb, stb );
    pkt->dts         =   pkt->pts;

    set_last_pts( pkt->pts );
}





/*******************************************************************************
SubEncode::set_last_pts()
********************************************************************************/
void    SubEncode::set_last_pts( int64_t _pts )
{
    last_pts    =   _pts;
}




/*******************************************************************************
SubEncode::set_last_pts()
********************************************************************************/
AVRational  SubEncode::get_compare_timebase()
{
    return  get_src_stream_timebase();
}



/*******************************************************************************
SubEncode::unref_data()
********************************************************************************/
void SubEncode::unref_data()
{
    unref_subtitle();
}



/*******************************************************************************
SubEncode::flush()
********************************************************************************/
int     SubEncode::flush()
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
    avsubtitle_free(&sub);
}







/*******************************************************************************
SubEncode::get_subtitle_pts()
********************************************************************************/
int64_t     SubEncode::get_subtitle_pts()
{
    return  sub.pts;
}

