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
    flush_state    =   false;

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

    if( sws_ctx != nullptr )
    {
        sws_freeContext( sws_ctx );
        sws_ctx     =   nullptr;
    }

    flush_state    =   false;
    codec   =   nullptr;
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
Encode::get_pkt()
********************************************************************************/
AVPacket*   Encode::get_pkt()
{
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
