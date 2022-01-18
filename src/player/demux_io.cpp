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
    IO->close();

    // 看討論底下的資源會在 Demux::end() 釋放
    // 手動釋放會 crash.
    // https://github.com/FFmpeg/FFmpeg/blob/d61240f8c95e9cf7a0aaef2bb4495960d3fec62c/libavformat/demux.c
    // 可以參考 source code. 
    /*if( input_buf != nullptr )
    {
        av_free(input_buf);
        input_buf = nullptr;
    }
    if( io_ctx != nullptr )
    {
        avio_context_free(&io_ctx);
        io_ctx = nullptr;
    }*/

    // need add release code.

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
	
    assert( input_buf == nullptr );
    input_buf   =   (uint8_t*)av_malloc(FFMPEG_INPUT_BUFFER_SIZE);

    assert( io_ctx == nullptr );
	io_ctx      =   avio_alloc_context( input_buf, FFMPEG_INPUT_BUFFER_SIZE, 0, (void*)IO, io_read_data, nullptr, nullptr );

    ret         =   av_probe_input_buffer( io_ctx, &input_fmt, nullptr, nullptr, 0, 0 );
    assert( ret == 0 );
	fmt_ctx->pb =   io_ctx;
    ret         =   avformat_open_input( &fmt_ctx, nullptr, input_fmt, nullptr );
    assert( ret == 0 );

    fmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;  

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
