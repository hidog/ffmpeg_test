#include "mux.h"
#include "tool.h"

extern "C" {

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

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
    v_stream    =   avformat_new_stream( output_ctx, v_codec ); // 未來改成從encode傳入
    if( v_stream == nullptr )
        MYLOG( LOG::ERROR, "v_stream is nullptr." );
    v_stream->id    =   output_ctx->nb_streams - 1;         // 未來改掉這邊的寫法. 增加彈性.
    v_stream->time_base.num     =   v_ctx->time_base.num;   // 在某個操作後這邊的 value 會變.
    v_stream->time_base.den     =   v_ctx->time_base.den;

    // audio
    a_stream    =   avformat_new_stream( output_ctx, a_codec );
    if( a_stream == nullptr )
        MYLOG( LOG::ERROR, "a_stream is nullptr." );
    a_stream->id    =   output_ctx->nb_streams - 1;
    a_stream->time_base.num     =   a_ctx->time_base.num;
    a_stream->time_base.den     =   a_ctx->time_base.den;

    //
    if( output_ctx->oformat->flags & AVFMT_GLOBALHEADER )
    {
        // 測了一下, 沒加這行還是還是能播放
        v_ctx->flags    |=  AV_CODEC_FLAG_GLOBAL_HEADER;
        a_ctx->flags    |=  AV_CODEC_FLAG_GLOBAL_HEADER;
    }
}






/*******************************************************************************
Mux::close_stream()
********************************************************************************/
void Mux::end()
{
    avformat_free_context(output_ctx);
}




/*******************************************************************************
Mux::init()
********************************************************************************/
void Mux::init( EncodeSetting setting, AVCodecContext* v_ctx, AVCodec *v_codec, AVCodecContext* a_ctx, AVCodec *a_codec )
{
    int             ret;
    AVDictionary    *opt    =   nullptr;

    // alloc output.
    //avformat_alloc_output_context2( &output_ctx, NULL, NULL, setting.filename.c_str() );
    avformat_alloc_output_context2( &output_ctx, NULL, setting.extension.c_str(), setting.filename.c_str() );
    if( output_ctx == nullptr ) 
        MYLOG( LOG::ERROR, "output_ctx = nullptr" );

    // 
    MYLOG( LOG::INFO, "default video codec is %s, use %s", avcodec_get_name(output_ctx->oformat->video_codec), avcodec_get_name(v_ctx->codec_id) );
    MYLOG( LOG::INFO, "default audio codec is %s, use %s", avcodec_get_name(output_ctx->oformat->audio_codec), avcodec_get_name(a_ctx->codec_id) );
    output_ctx->oformat->video_codec    =   v_ctx->codec_id;
    output_ctx->oformat->audio_codec    =   a_ctx->codec_id;

    // add stream
    add_stream( v_ctx, v_codec, a_ctx, a_codec );

    ret =   avcodec_parameters_from_context( v_stream->codecpar, v_ctx );
    assert( ret == 0 );
    ret =   avcodec_parameters_from_context( a_stream->codecpar, a_ctx );
    assert( ret == 0 );

    // 研究一下這邊跟範例程式的差異
    av_dump_format( output_ctx, 0, setting.filename.c_str(), 1 );

    //
    if( 0 == (output_ctx->oformat->flags & AVFMT_NOFILE) )
    {
        ret     =   avio_open( &output_ctx->pb, setting.filename.c_str() , AVIO_FLAG_WRITE );
        if( ret < 0 ) 
            MYLOG( LOG::ERROR, "open output file fail" );
    }
}







/*******************************************************************************
Mux::write_header()
********************************************************************************/
void Mux::write_header()
{
    AVDictionary    *opt    =   nullptr; // 未來改成class member. 方便寫入參數 
    int ret =   avformat_write_header( output_ctx, &opt );

    char tmp[AV_ERROR_MAX_STRING_SIZE];


    if( ret < 0 )
        //MYLOG( LOG::ERROR, "write header fail. err = %d", ret );
        MYLOG( LOG::ERROR, "write header fail. err = %s", av_make_error_string(tmp,AV_ERROR_MAX_STRING_SIZE,ret) );

}




/*******************************************************************************
Mux::write_frame()
********************************************************************************/
void Mux::write_frame( AVPacket* pkt )
{
    int ret =   av_interleaved_write_frame( output_ctx, pkt );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "write fail." );
}




/*******************************************************************************
Mux::write_end()
********************************************************************************/
void Mux::write_end()
{
    av_write_trailer(output_ctx);

    if( 0 == ( output_ctx->oformat->flags & AVFMT_NOFILE) )
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