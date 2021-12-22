#include "mux.h"



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tool.h"

extern "C" {

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

}






/*******************************************************************************
Mux::Mux()
********************************************************************************/
Mux::Mux()
{}




/*******************************************************************************
Mux::Mux()
********************************************************************************/
Mux::~Mux()
{}





/*******************************************************************************
Mux::add_stream()
********************************************************************************/
void    Mux::add_stream( AVCodecContext* v_ctx, AVCodec *v_codec, AVCodecContext* a_ctx, AVCodec *a_codec )
{
    // video
    v_stream = avformat_new_stream( output_ctx, v_codec ); // 未來改成從encode傳入
    if( v_stream == nullptr )
        MYLOG( LOG::ERROR, "v_stream is nullptr." );
    v_stream->id = output_ctx->nb_streams - 1;  // 未來改掉這邊的寫法. 增加彈性.
    v_stream->time_base.num = v_ctx->time_base.num;  // 在某個操作後這邊的 value 會變.
    v_stream->time_base.den = v_ctx->time_base.den;

    // audio
    a_stream = avformat_new_stream( output_ctx, a_codec );
    if( a_stream == nullptr )
        MYLOG( LOG::ERROR, "a_stream is nullptr." );
    a_stream->id = output_ctx->nb_streams - 1;
    a_stream->time_base.num = a_ctx->time_base.num;
    a_stream->time_base.den = a_ctx->time_base.den;

    // 未來看是否需要加入這段code
    if( output_ctx->oformat->flags & AVFMT_GLOBALHEADER )
        MYLOG( LOG::DEBUG, "for test" );
        //  c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}




/*******************************************************************************
Mux::close_stream()
********************************************************************************/
void Mux::end()
{
    /* free the stream */
    avformat_free_context(output_ctx);
}




/*******************************************************************************
Mux::init()
********************************************************************************/
void Mux::init( AVCodecContext* v_ctx, AVCodec *v_codec, AVCodecContext* a_ctx, AVCodec *a_codec )
{
    int             ret;
    AVDictionary    *opt    =   nullptr;

    // alloc output.
    avformat_alloc_output_context2( &output_ctx, NULL, "mp4", "J:\\test.mp4" );
    //avformat_alloc_output_context2( &output_ctx, NULL, NULL, "H:\\test.mp4" );
    if( output_ctx == nullptr ) 
        MYLOG( LOG::ERROR, "output_ctx = nullptr" );

    output_fmt  =   output_ctx->oformat;

    /*
    看改h265或其他編碼是不是要跟著修這個參數
    output_fmt->video_codec
    output_fmt->audio_codec
    */

    // add stream
    add_stream( v_ctx, v_codec, a_ctx, a_codec );

    ret =   avcodec_parameters_from_context( v_stream->codecpar, v_ctx );
    assert(ret);
    ret =   avcodec_parameters_from_context( a_stream->codecpar, a_ctx );
    assert(ret);

    // 研究一下這邊跟範例程式的差異
    av_dump_format( output_ctx, 0, "J:\\test.mp4", 1 );

    /* open the output file, if needed */
    if( !(output_fmt->flags & AVFMT_NOFILE) )
    {
        ret = avio_open( &output_ctx->pb, "J:\\test.mp4", AVIO_FLAG_WRITE );
        if( ret < 0 ) 
            MYLOG( LOG::ERROR, "open output file fail" );
    }
}







/*******************************************************************************
Mux::write_header()
********************************************************************************/
void Mux::write_header()
{
    AVDictionary *opt = nullptr; // 未來改成class member. 方便寫入參數 
    int ret = avformat_write_header( output_ctx, &opt );
    if (ret < 0) 
        MYLOG( LOG::ERROR, "write header fail. err = %d", ret );
}




/*******************************************************************************
Mux::write_frame()
********************************************************************************/
void Mux::write_frame( AVPacket* pkt )
{
    int ret = av_interleaved_write_frame( output_ctx, pkt );
    if (ret < 0) 
        MYLOG( LOG::ERROR, "write fail." );
}




/*******************************************************************************
Mux::write_end()
********************************************************************************/
void Mux::write_end()
{

    av_write_trailer(output_ctx);

    if ( !(output_fmt->flags & AVFMT_NOFILE) )
        avio_closep( &output_ctx->pb );
}





/*******************************************************************************
Mux::get_video_stream_timebase()
********************************************************************************/
AVRational  Mux::get_video_stream_timebase()
{
    if( v_stream == nullptr )
        MYLOG( LOG::ERROR, "v stream is null." );
    return  v_stream->time_base;
}






/*******************************************************************************
Mux::get_audio_stream_timebase()
********************************************************************************/
AVRational  Mux::get_audio_stream_timebase()
{
    if( a_stream == nullptr )
        MYLOG( LOG::ERROR, "a stream is null." );
    return  a_stream->time_base;
}