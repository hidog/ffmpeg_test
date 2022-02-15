#include "encode.h"
#include "tool.h"


extern "C" {

#include <libavcodec/avcodec.h>

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

#ifdef FFMPEG_TEST
    // init frame
    if( frame != nullptr )
        av_frame_free( &frame );
    frame   =   av_frame_alloc();
    if( frame == nullptr ) 
        MYLOG( LOG::L_ERROR, "frame alloc fail." );
    frame->pts  =   0;
#endif    

    // init pkt
    if( pkt != nullptr )    
        av_packet_free( &pkt );       
    
    pkt     =   av_packet_alloc();
    if( pkt == nullptr )
        MYLOG( LOG::L_ERROR, "pkt = nullptr." );

    // codec
    codec   =   avcodec_find_encoder(code_id);
    if( codec == nullptr )
        MYLOG( LOG::L_ERROR, "codec not find. code id = %s", avcodec_get_name(code_id) );

    // init codec ctx.
    if( ctx != nullptr )
        avcodec_free_context( &ctx );

    ctx     =   avcodec_alloc_context3( codec );
    if( ctx == nullptr ) 
        MYLOG( LOG::L_ERROR, "ctx = nullptr." );
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
    frame_count =   0;
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
Encode::set_stream_time_base()
********************************************************************************/
AVRational  Encode::get_compare_timebase()
{
    return  get_timebase();
}


    
/*******************************************************************************
Encode::get_stream_time_base()
********************************************************************************/
AVRational  Encode::get_stream_time_base()
{
    if( stream_time_base.num == 0 && stream_time_base.den == 0 )
        MYLOG( LOG::L_ERROR, "need set stream time base" );
    return  stream_time_base;
}



#if 0
/*******************************************************************************
Encode::unref_pkt()
********************************************************************************/
void    Encode::unref_pkt()
{
    av_packet_unref(pkt);
}
#endif



/*******************************************************************************
Encode::unref_data()
********************************************************************************/
void    Encode::unref_data()
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
        MYLOG( LOG::L_ERROR, "frame or ctx is null." );
        return  R_ERROR;
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
        MYLOG( LOG::L_ERROR, "ctx or pkt == nullptr." );

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
Encode::encode_timestamp()
********************************************************************************/
void    Encode::encode_timestamp()
{
    if( pkt == nullptr )
        MYLOG( LOG::L_WARN, "pkt is null." );
    auto ctx_tb =   get_timebase();
    auto stb    =   get_stream_time_base();
    av_packet_rescale_ts( pkt, ctx_tb, stb );
}





/*******************************************************************************
Encode::get_pkt()
********************************************************************************/
AVPacket*   Encode::get_pkt()
{
    if( pkt == nullptr )
        MYLOG( LOG::L_WARN, "pkt is null." );
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
        MYLOG( LOG::L_ERROR, "ctx is null." );
        return  R_ERROR;
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
        //MYLOG( LOG::L_ERROR, "both eof." );
        return  true;
    }
    else if( a.end_of_file() == true )
        return  false;
    else if( b.end_of_file() == true )
        return  true;

    int64_t     a_pts   =   a.get_pts();
    int64_t     b_pts   =   b.get_pts();

    AVRational  a_tb    =   a.get_compare_timebase();
    AVRational  b_tb    =   b.get_compare_timebase();

    int     ret     =   av_compare_ts( a_pts, a_tb, b_pts, b_tb );
    if( ret <= 0 )
        return  true;
    else
        return  false;
}







/*******************************************************************************
Encode::get_frame()
********************************************************************************/
AVFrame*    Encode::get_frame()
{
    if( frame == nullptr )
        MYLOG( LOG::L_WARN, "frame is null" );
    return  frame;
}
