#include "mux_io.h"

#include "tool.h"


extern "C" {

#include <libavformat/avformat.h>

} // end extern "C"





/*******************************************************************************
MuxIO::MuxIO()
********************************************************************************/
MuxIO::MuxIO()
    :   Mux()
{}




/*******************************************************************************
MuxIO::MuxIO()
********************************************************************************/
MuxIO::~MuxIO()
{
    end();
}





/*******************************************************************************
MuxIO::init()
********************************************************************************/
void    MuxIO::init( EncodeSetting setting )
{
    assert( output_buf == nullptr );
    output_buf  =   (uint8_t*)av_malloc(FFMPEG_OUTPUT_BUFFER_SIZE);

    // alloc output.
    avformat_alloc_output_context2( &output_ctx, NULL, "mpegts", nullptr );
    if( output_ctx == nullptr ) 
        MYLOG( LOG::ERROR, "output_ctx = nullptr" );

    // 
    MYLOG( LOG::INFO, "default video codec is %s", avcodec_get_name(output_ctx->oformat->video_codec) );
    MYLOG( LOG::INFO, "default audio codec is %s", avcodec_get_name(output_ctx->oformat->audio_codec) );

    // add stream
    // video
    v_stream    =   avformat_new_stream( output_ctx, nullptr );  // 第二個參數丟 nullptr 即可
    if( v_stream == nullptr )
        MYLOG( LOG::ERROR, "v_stream is nullptr." );
    v_stream->id    =   output_ctx->nb_streams - 1;         // 未來改掉這邊的寫法. 增加彈性. 方便增加多個 audio stream, subtitle stream
    
    // audio
    a_stream    =   avformat_new_stream( output_ctx, nullptr );
    if( a_stream == nullptr )
        MYLOG( LOG::ERROR, "a_stream is nullptr." );
    a_stream->id    =   output_ctx->nb_streams - 1;
}






/*******************************************************************************
MuxIO::end()
********************************************************************************/
void    MuxIO::end()
{
    if( io_ctx != nullptr )
    {
        // avio_close( io_ctx ); 呼叫這行會 crash
        avio_context_free( &io_ctx );
        io_ctx  =   nullptr;
    }

    if( output_buf != nullptr )
    {
        av_free( output_buf );
        output_buf  =   nullptr;
    }

    Mux::end();
}








/*******************************************************************************
io_write_data
********************************************************************************/
int     io_write_data( void *opaque, uint8_t *buffer, int size )
{
    InputOutput*    io  =   (InputOutput*)opaque;
    int     ret     =   io->write( buffer, size );
    return  ret;
}






/*******************************************************************************
MuxIO::write_end()
********************************************************************************/
void    MuxIO::write_end()
{
    av_write_trailer(output_ctx);
}






/*******************************************************************************
MuxIO::open()
********************************************************************************/
void    MuxIO::open( EncodeSetting setting, AVCodecContext* v_ctx, AVCodecContext* a_ctx, InputOutput *IO )
{
    int     ret     =   0;
	
    io_ctx  =   avio_alloc_context( output_buf, FFMPEG_OUTPUT_BUFFER_SIZE, 1, (void*)IO, nullptr, io_write_data, nullptr );
    if( io_ctx == nullptr )
        MYLOG( LOG::ERROR, "io_ctx is null." );
    
    // 測試用
    ret     =   av_dict_set( &(output_ctx->metadata), "service_name", "hidog test", 0 );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "set service_name fail." );
    
    output_ctx->pb      =   io_ctx;
    output_ctx->flags   =   AVFMT_FLAG_CUSTOM_IO;

    if( v_ctx == nullptr || a_ctx == nullptr )
        MYLOG( LOG::ERROR, "v ctx or a ctx is null" );

    // copy time base.
    v_stream->time_base     =   v_ctx->time_base;   // 在某個操作後這邊的 value 會變.
    a_stream->time_base     =   a_ctx->time_base;
    
    // 
    ret     =   avcodec_parameters_from_context( v_stream->codecpar, v_ctx );
    assert( ret == 0 );
    ret     =   avcodec_parameters_from_context( a_stream->codecpar, a_ctx );
    assert( ret == 0 );
}




