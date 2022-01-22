#include "demux.h"

extern "C" {

#include <libavformat/avformat.h>

} // end extern "C"





/*******************************************************************************
Demux::Demux()
********************************************************************************/
Demux::Demux()
{}




/*******************************************************************************
Demux::~Demux()
********************************************************************************/
Demux::~Demux()
{
    end();
}







/*******************************************************************************
Demux::init()
********************************************************************************/
int    Demux::init()
{
    int     ret;

    /*
        use for multi-thread
    */
#ifdef USE_MT
    for( int i = 0; i < pkt_size; i++ )
    {
        pkt_array[i]    =   av_packet_alloc();
        
        if( pkt_array[i] == nullptr )
        {
            ret     =   AVERROR(ENOMEM);
            MYLOG( LOG::L_ERROR, "Could not allocate packet. error = %d", ret );
            return  R_ERROR; 
        }

        pkt_queue.emplace( pkt_array[i] );        
    }
#endif

    /*
        av_init_packet(&pkt);
        如果不是宣告為指標,可以用這個函數來初始化
    */
    pkt     =   av_packet_alloc();
    if( pkt == nullptr )
    {
        ret     =   AVERROR(ENOMEM);
        MYLOG( LOG::L_ERROR, "Could not allocate packet. error = %d", ret );
        return  R_ERROR;
    }

    return  R_SUCCESS;
}






/*******************************************************************************
Demux::set_input_file()
********************************************************************************/
void    Demux::set_input_file( std::string fn )
{
    filename    =   fn;
}




/*******************************************************************************
Demux::stream_info()
********************************************************************************/
int     Demux::stream_info()
{
    int ret =   0;

    //
    ret     =   avformat_find_stream_info( fmt_ctx, nullptr );
    if( ret < 0) 
    {
        MYLOG( LOG::L_ERROR, "Could not find stream information. ret = %d", ret );
        return  R_ERROR;
    }

    MYLOG( LOG::L_INFO, "nb_streams = %d", fmt_ctx->nb_streams );

    return  R_SUCCESS;
}






/*******************************************************************************
Demux::end()
********************************************************************************/
int     Demux::end()
{
    if( fmt_ctx != nullptr )
    {
        avformat_close_input( &fmt_ctx );
        fmt_ctx     =   nullptr;
    }

    if( pkt != nullptr )
    {
        av_packet_free( &pkt );
        pkt         =   nullptr;
    }

    //
#ifdef USE_MT
    while( pkt_queue.empty() == false )
        pkt_queue.pop();
    for( int i = 0; i < pkt_size; i++ )
        av_packet_free( &pkt_array[i] );
#endif

    return  R_SUCCESS;
}






/*******************************************************************************
Demux::open_input()
********************************************************************************/
int     Demux::open_input()
{
    fmt_ctx     =   avformat_alloc_context();    
    int  ret    =   0;

    MYLOG( LOG::L_INFO, "load file %s", filename.c_str() );
    ret     =   avformat_open_input( &fmt_ctx, filename.c_str(), NULL, NULL );

    if( ret < 0 )
    {
        MYLOG( LOG::L_ERROR, "Could not open source file %s", filename.c_str() );
        return  R_ERROR;
    }

    // 
    ret     =   stream_info();
    if( ret == R_ERROR )
    {
        MYLOG( LOG::L_ERROR, "init fail. ret = %d", ret );
        return  R_ERROR;
    }

    // dump input information to stderr 
    av_dump_format( fmt_ctx, 0, filename.c_str(), 0 );

    return  R_SUCCESS;
}










/*******************************************************************************
Demux::get_format_context()
********************************************************************************/
AVFormatContext*    Demux::get_format_context()
{
    return  fmt_ctx;
}






/*******************************************************************************
Demux::get_pkt()
********************************************************************************/
AVPacket*   Demux::get_packet()
{
    return  pkt;
}



/*******************************************************************************
Demux::unref_pkt()
********************************************************************************/
void    Demux::unref_packet()
{
    av_packet_unref(pkt);
}



/*******************************************************************************
Demux::demux()
********************************************************************************/
int    Demux::demux()
{
    int     ret;
    ret     =   av_read_frame( fmt_ctx, pkt );

    //if( ret < 0 )    
      //  MYLOG( LOG::L_INFO, "load file end." );

    return ret;
}







#ifdef USE_MT
/*******************************************************************************
Demux::collect_packet()
********************************************************************************/
void    Demux::collect_packet( AVPacket *_pkt )
{
    std::lock_guard<std::mutex>     lock(pkt_mtx);
    av_packet_unref(_pkt);
    pkt_queue.push(_pkt);
}
#endif







#ifdef USE_MT
/*******************************************************************************
Demux::demux_multi_thread()
********************************************************************************/
std::pair<int,AVPacket*>     Demux::demux_multi_thread()
{
    int     ret;
    AVPacket    *packet     =   nullptr;

    if( pkt_queue.empty() == true )
    {
        MYLOG( LOG::L_ERROR, "pkt stack empty." );
        return  std::make_pair( 0, nullptr );
    }

    if( pkt_queue.size() < 10 )
        MYLOG( LOG::L_WARN, "queue size = %d", pkt_queue.size() );

    pkt_mtx.lock();
    packet  =   pkt_queue.front();
    pkt_queue.pop();
    pkt_mtx.unlock();

    ret     =   av_read_frame( fmt_ctx, packet );

    if( ret < 0 )    
        MYLOG( LOG::L_INFO, "load file end." );

    std::pair<int,AVPacket*>    result  =   std::make_pair( ret, packet );

    return  result;
}
#endif



/*******************************************************************************
Demux::get_duration_time()
********************************************************************************/
int64_t     Demux::get_duration_time()
{
    if( fmt_ctx->duration == AV_NOPTS_VALUE )
    {
        MYLOG( LOG::L_WARN, "not defined duration");
        return  INT64_MAX;
    }

    int64_t     duration    =   fmt_ctx->duration + (fmt_ctx->duration <= INT64_MAX - 5000 ? 5000 : 0);
    int64_t     secs        =   duration / AV_TIME_BASE;
    int64_t     us          =   duration % AV_TIME_BASE;

    if( us > 0 )
        secs++;

    return  secs;
}


