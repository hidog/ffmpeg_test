#include "demux_io.h"


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






int     read_data( void *opaque, uint8_t *buf, int buf_size )
{
    static FILE *fp = fopen( "D:\\code\\test.mkv", "rb");

    int ret = fread( buf, 1, 4096, fp );

    if( buf_size != 4096 )
        printf( "read %d\n", buf_size );

    return ret;
}








/*******************************************************************************
DemuxIO::open_input()
********************************************************************************/
int     DemuxIO::open_input()
{
    fmt_ctx     =   avformat_alloc_context();    
    int  ret    =   0;


	AVInputFormat*  input_fmt   =   nullptr;
	uint8_t*        input_buf   =   (uint8_t*)av_malloc(4096);
	AVIOContext*    io_ctx      =   avio_alloc_context( input_buf, 4096, 0, this, read_data, NULL, NULL );

    ret         =   av_probe_input_buffer( io_ctx, &input_fmt, "", nullptr, 0, 0 );
	fmt_ctx->pb =   io_ctx;
    ret         =   avformat_open_input( &fmt_ctx, "", input_fmt, NULL );

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

