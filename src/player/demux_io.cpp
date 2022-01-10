#include "demux_io.h"
#include "player.h"
#include "../IO/input_output.h"


extern "C" {

#include <libavformat/avformat.h>

} // end extern "C"






/*******************************************************************************
DemuxIO::DemuxIO()
********************************************************************************/
DemuxIO::DemuxIO()
    :   Demux()
{}






/*******************************************************************************
DemuxIO::~DemuxIO()
********************************************************************************/
DemuxIO::~DemuxIO()
{}








/*******************************************************************************
DemuxIO::open_input()
********************************************************************************/
int     DemuxIO::open_input()
{
    fmt_ctx     =   avformat_alloc_context();    
    int  ret    =   0;

	AVInputFormat*  input_fmt   =   nullptr;
	uint8_t*        input_buf   =   (uint8_t*)av_malloc(4096);
	AVIOContext*    io_ctx      =   avio_alloc_context( input_buf, 4096, 0, (void*)IO, io_read_data, nullptr, nullptr );

    ret         =   av_probe_input_buffer( io_ctx, &input_fmt, nullptr, nullptr, 0, 0 );
	fmt_ctx->pb =   io_ctx;
    ret         =   avformat_open_input( &fmt_ctx, nullptr, input_fmt, nullptr );

    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "Could not open" );
        return  ERROR;
    }

    // 
    ret     =   stream_info();
    if( ret == ERROR )
    {
        MYLOG( LOG::ERROR, "init fail. ret = %d", ret );
        return  ERROR;
    }

    return  SUCCESS;
}





/*******************************************************************************
DemuxIO::set_IO()
********************************************************************************/
void    DemuxIO::set_IO( InputOutput* _io )
{
    IO  =   _io;
}
