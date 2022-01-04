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
Mux::close_stream()
********************************************************************************/
void Mux::end()
{
    avformat_free_context(output_ctx);
}




/*******************************************************************************
Mux::init()
********************************************************************************/
void Mux::init( EncodeSetting setting )
{
    int             ret;
    AVDictionary    *opt    =   nullptr;

    // alloc output.
    //avformat_alloc_output_context2( &output_ctx, NULL, NULL, setting.filename.c_str() );  //副檔名參數可以不用傳進去,讓系統自動判斷
    avformat_alloc_output_context2( &output_ctx, NULL, setting.extension.c_str(), setting.filename.c_str() );
    if( output_ctx == nullptr ) 
        MYLOG( LOG::ERROR, "output_ctx = nullptr" );

    // 
    MYLOG( LOG::INFO, "default video codec is %s", avcodec_get_name(output_ctx->oformat->video_codec) );
    MYLOG( LOG::INFO, "default audio codec is %s", avcodec_get_name(output_ctx->oformat->audio_codec) );
    MYLOG( LOG::INFO, "default audio codec is %s", avcodec_get_name(output_ctx->oformat->subtitle_codec) );


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

    // subtitle
    if( setting.has_subtitle == true )
    {
        s_stream    =   avformat_new_stream( output_ctx, nullptr );
        if( s_stream == nullptr )
            MYLOG( LOG::ERROR, "a_stream is nullptr." );
        s_stream->id    =   output_ctx->nb_streams - 1;
    }


}







/*******************************************************************************
Mux::open()
********************************************************************************/
void    Mux::open( EncodeSetting setting, AVCodecContext* v_ctx, AVCodecContext* a_ctx, AVCodecContext* s_ctx )
{
    if( v_ctx == nullptr || a_ctx == nullptr )
        MYLOG( LOG::ERROR, "v ctx or a ctx is null" );

    if( setting.has_subtitle == true && s_ctx == nullptr )
        MYLOG( LOG::ERROR, "subtitle ctx is null." );   

    // copy time base.
    v_stream->time_base     =   v_ctx->time_base;   // 在某個操作後這邊的 value 會變.
    a_stream->time_base     =   a_ctx->time_base;
    if( setting.has_subtitle == true )
        s_stream->time_base     =   s_ctx->time_base;

    //
    int     ret     =   0;

    // after open
    ret =   avcodec_parameters_from_context( v_stream->codecpar, v_ctx );
    assert( ret == 0 );
    ret =   avcodec_parameters_from_context( a_stream->codecpar, a_ctx );
    assert( ret == 0 );
    if( setting.has_subtitle == true )
    {
        ret =   avcodec_parameters_from_context( s_stream->codecpar, s_ctx );
        assert( ret == 0 );
    }

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
Mux::is_need_global_header()
********************************************************************************/
bool    Mux::is_need_global_header()
{
    return output_ctx->oformat->flags & AVFMT_GLOBALHEADER;
}





/*******************************************************************************
Mux::write_header()
********************************************************************************/
void Mux::write_header()
{
    AVDictionary    *opt    =   nullptr; // 未來改成class member. 方便寫入參數 
    int ret =   avformat_write_header( output_ctx, &opt );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "write header fail. err = %d", ret );
        //MYLOG( LOG::ERROR, "write header fail. err = %s", av_make_error_string(tmp,AV_ERROR_MAX_STRING_SIZE,ret) );
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
Mux::write_frame()
********************************************************************************/
void    Mux::write_subtitle( AVPacket* pkt )
{
    int     ret     =   0;

    if( pkt->size != 0 )
        ret =   av_write_frame( output_ctx, pkt );   // 沒找到相關說明...
    else 
        ret =   av_interleaved_write_frame( output_ctx, pkt );  
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






/*******************************************************************************
Mux::get_sub_stream_timebase()
********************************************************************************/
AVRational  Mux::get_sub_stream_timebase()
{
    if( s_stream == nullptr )
        MYLOG( LOG::ERROR, "a stream is null." );
    return  s_stream->time_base;
}