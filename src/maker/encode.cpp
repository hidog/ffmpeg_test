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

    // init frame
    if( frame == nullptr )
        av_frame_free( &frame );

    frame   =   av_frame_alloc();
    if( frame == nullptr ) 
        MYLOG( LOG::ERROR, "frame alloc fail." );

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

    codec   =   nullptr;
}