#include "demux_io.h"
#include "player.h"
#include "../IO/input_output.h"


extern "C" {

#include <libavformat/avformat.h>

} // end extern "C"






/*******************************************************************************
DemuxIO::DemuxIO()
********************************************************************************/
DemuxIO::DemuxIO( DecodeSetting _st )
    :   Demux()
{
    setting     =   _st;
    IO          =   create_IO( setting.io_type, IO_Direction::RECV );
    IO->set_decode( setting );
}






/*******************************************************************************
DemuxIO::~DemuxIO()
********************************************************************************/
DemuxIO::~DemuxIO()
{}






/*******************************************************************************
DemuxIO::init()
********************************************************************************/
int     DemuxIO::init()
{
    IO->init();
    int ret     =   Demux::init();
    return  ret;
}






/*******************************************************************************
DemuxIO::end()
********************************************************************************/
int    DemuxIO::end()
{
    // need add release code.

    IO->close();
    Demux::end();

    return  SUCCESS;
}






/*******************************************************************************
DemuxIO::open_input()
********************************************************************************/
int     DemuxIO::open_input()
{
    IO->open();

    fmt_ctx     =   avformat_alloc_context();    
    int  ret    =   0;

	AVInputFormat*  input_fmt   =   nullptr;
	AVIOContext*    io_ctx      =   avio_alloc_context( input_buf, FFMPEG_INPUT_BUFFER_SIZE, 0, (void*)IO, io_read_data, nullptr, nullptr );

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
