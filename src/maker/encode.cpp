#include "encode.h"
#include "tool.h"


extern "C" {

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

} // end extern "C"







/*******************************************************************************
Encode::Encode()
********************************************************************************/
Encode::Encode()
{}




/*******************************************************************************
Encode::~Encode()
********************************************************************************/
Encode::~Encode()
{}




/*******************************************************************************
Encode::init()
********************************************************************************/
void    Encode::init( int st_idx, AVCodecID code_id )
{
    stream_index    =   st_idx;
    flush_state     =   false;
    eof_flag        =   false;

    // init frame
    if( frame != nullptr )
        av_frame_free( &frame );

    frame   =   av_frame_alloc();
    if( frame == nullptr ) 
        MYLOG( LOG::ERROR, "frame alloc fail." );
    frame->pts  =   0;

    // init pkt
    if( pkt != nullptr )    
        av_packet_free( &pkt );       
    
    pkt     =   av_packet_alloc();
    if( pkt == nullptr )
        MYLOG( LOG::ERROR, "pkt = nullptr." );

    // codec
    codec   =   avcodec_find_encoder(code_id);
    if( codec == nullptr )
        MYLOG( LOG::ERROR, "codec not find. code id = %s", avcodec_get_name(code_id) );

    // init codec ctx.
    if( ctx != nullptr )
        avcodec_free_context( &ctx );

    ctx     =   avcodec_alloc_context3( codec );
    if( ctx == nullptr ) 
        MYLOG( LOG::ERROR, "ctx = nullptr." );
}






/*******************************************************************************
Encode::init()
********************************************************************************/
void    Encode::end()
{
    if( ctx != nullptr )
    {
        avcodec_free_context( &ctx );
        ctx     =   nullptr;
    }

    if( frame != nullptr )
    {
        av_frame_free( &frame );
        frame   =   nullptr;
    }

    if( pkt == nullptr )
    {
        av_packet_free( &pkt );
        pkt     =   nullptr;
    }

    flush_state =   false;
    eof_flag    =   false;
    codec       =   nullptr;
}




/*******************************************************************************
Encode::end_of_file()
********************************************************************************/
bool    Encode::end_of_file()
{
    return  eof_flag;
}






/*******************************************************************************
Encode::set_stream_time_base()
********************************************************************************/
void    Encode::set_stream_time_base( AVRational _stb )
{
    stream_time_base    =   _stb;
}



    
/*******************************************************************************
Encode::get_stream_time_base()
********************************************************************************/
AVRational  Encode::get_stream_time_base()
{
    if( stream_time_base.num == 0 && stream_time_base.den == 0 )
        MYLOG( LOG::ERROR, "need set stream time base" );
    return  stream_time_base;
}




/*******************************************************************************
Encode::unref_pkt()
********************************************************************************/
void    Encode::unref_pkt()
{
    av_packet_unref(pkt);
}






/*******************************************************************************
Encode::is_flush()
********************************************************************************/
bool    Encode::is_flush()
{
    return  flush_state;
}



/*******************************************************************************
Encode::set_flush()
********************************************************************************/
void    Encode::set_flush( bool flag )
{
    flush_state =   flag;
}


/*******************************************************************************
Encode::send_frame()
********************************************************************************/
int     Encode::send_frame()
{
    if( frame == nullptr || ctx == nullptr )
    {
        MYLOG( LOG::ERROR, "frame or ctx is null." );
        return  ERROR;
    }

    //printf("send frame %d\n", frame->data[0][100] );

    int ret =   avcodec_send_frame( ctx, frame );
    return  ret;
}





/*******************************************************************************
Encode::recv_frame()
********************************************************************************/
int     Encode::recv_frame()
{
    if( ctx == nullptr || pkt == nullptr )
        MYLOG( LOG::ERROR, "ctx or pkt == nullptr." );

    int     ret;

    ret                 =   avcodec_receive_packet( ctx, pkt );
    pkt->stream_index   =   stream_index;

    return  ret;
}






/*******************************************************************************
Encode::set_frame()
********************************************************************************/
void    Encode::set_frame( AVFrame* _f )
{
    //av_frame_copy( frame, _f );
    //av_frame_free( &_f );
    frame   =   _f;
}




/*******************************************************************************
Encode::get_pkt()
********************************************************************************/
AVPacket*   Encode::get_pkt()
{
    if( pkt == nullptr )
        MYLOG( LOG::WARN, "pkt is null." );
    return  pkt;   
}




/*******************************************************************************
Encode::get_codec()
********************************************************************************/
AVCodec*    Encode::get_codec()
{
    return  codec;
}





/*******************************************************************************
Encode::get_ctx()
********************************************************************************/
AVCodecContext*     Encode::get_ctx()
{
    return  ctx;
}




/*******************************************************************************
Encode::get_ctx()
********************************************************************************/
AVRational  Encode::get_timebase()
{
    return  ctx->time_base;
}





/*******************************************************************************
Encode::flush()
********************************************************************************/
int     Encode::flush()
{
    if( ctx == nullptr )
    {
        MYLOG( LOG::ERROR, "ctx is null." );
        return  ERROR;
    }

    int ret =   avcodec_send_frame( ctx, nullptr );
    return  ret;
}




/*******************************************************************************
Encode::is_empty()

判斷 encoder 已經沒資料
********************************************************************************/
bool    Encode::is_empty()
{
    if( frame == nullptr )
        return  true;
    else
        return  false;
}




/*******************************************************************************
operator <=

用來決定誰先進去mux
********************************************************************************/
bool    operator <= ( Encode& a, Encode& b )
{
    if( a.end_of_file() == true && b.end_of_file() == true )
    {
        MYLOG( LOG::ERROR, "both eof." );
        return  true;
    }
    else if( a.end_of_file() == true )
        return  false;
    else if( b.end_of_file() == true )
        return  true;

    int64_t     a_pts   =   a.get_pts();
    int64_t     b_pts   =   b.get_pts();

    AVRational  a_tb    =   a.get_timebase();
    AVRational  b_tb    =   b.get_timebase();

    int     ret     =   av_compare_ts( a_pts, a_tb, b_pts, b_tb );
    if( ret <= 0 )
        return  true;
    else
        return  false;

#if 0

    int         ret     =   0;
    int64_t     v_pts, a_pts, s_pts;
    EncodeOrder order;

    // 理論上不用考慮兩個都是 nullptr 的 case,在 loop 控制就排除這件事情了.
    if( v_frame == nullptr ) 
    {
        // flush video.
        // 必須在這邊處理,等 loop 完再處理有機會造成 stream flush 的部分寫入時間錯誤.  
        // 例如聲音比影像短, 導致 flush 階段的 audio frame pts 在 video frame 前面, 但寫入卻在後面.
        if( v_encoder->is_flush() == false )
        {
            st_tb   =   muxer->get_video_stream_timebase();
            flush_func( v_encoder );
        }
    }
    else if( a_frame == nullptr )
    {
        if( a_encoder->is_flush() == false )
        {
            st_tb   =   muxer->get_audio_stream_timebase();
            flush_func( a_encoder );  
        }
    }

    // note: 如果需要支援多 audio/subtitle, 這邊需要重新設計
    if( v_frame == nullptr && a_frame == nullptr )
        order   =   EncodeOrder::SUBTITLE;
    else if( v_frame == nullptr && s_encoder->get_queue_size() == 0 )
        order   =   EncodeOrder::AUDIO;
    else if( a_frame == nullptr && s_encoder->get_queue_size() == 0 )
        order   =   EncodeOrder::VIDEO;
    else if( s_encoder->get_queue_size() == 0 )
    {
        v_pts   =   v_frame->pts;
        a_pts   =   a_frame->pts;
        ret     =   av_compare_ts( v_pts, v_time_base, a_pts, a_time_base );
        if( ret <= 0 )
            order   =   EncodeOrder::VIDEO;
        else
            order   =   EncodeOrder::AUDIO;
    }
    else if( v_frame == nullptr )
    {
        a_pts   =   a_frame->pts;
        s_pts   =   s_encoder->get_pts();
        ret     =   av_compare_ts( a_pts, a_time_base, s_pts, s_time_base );
        if( ret <= 0 )
            order   =   EncodeOrder::AUDIO;
        else
            order   =   EncodeOrder::SUBTITLE;
    }
    else if( a_frame == nullptr )
    {
        v_pts   =   v_frame->pts;
        s_pts   =   s_encoder->get_pts();
        ret     =   av_compare_ts( v_pts, v_time_base, s_pts, s_time_base );
        if( ret <= 0 )
            order   =   EncodeOrder::VIDEO;
        else
            order   =   EncodeOrder::SUBTITLE;
    }
    else
    {
        // 原本想用 INT64_MAX, 但會造成 overflow.
        v_pts   =   v_frame->pts;
        a_pts   =   a_frame->pts;
        s_pts   =   s_encoder->get_pts();
        ret     =   av_compare_ts( v_pts, v_time_base, a_pts, a_time_base );
        if( ret <= 0 )
        {
            ret     =   av_compare_ts( v_pts, v_time_base, s_pts, s_time_base );
            if( ret <= 0 )
                order   =   EncodeOrder::VIDEO;
            else
                order   =   EncodeOrder::SUBTITLE;
        }
        else
        {
            ret     =   av_compare_ts( a_pts, a_time_base, s_pts, s_time_base );
            if( ret <= 0 )
                order   =   EncodeOrder::AUDIO;
            else
                order   =   EncodeOrder::SUBTITLE;
        }
    }

    return  order;
#endif
}







/*******************************************************************************
Encode::get_frame()
********************************************************************************/
AVFrame*    Encode::get_frame()
{
    if( frame == nullptr )
        MYLOG( LOG::WARN, "frame is null" );
    return  frame;
}
