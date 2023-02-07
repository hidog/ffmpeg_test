#include "decode.h"

#include <cassert>

extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

} // end extern "C"





/*******************************************************************************
Decode::Decode()
********************************************************************************/
Decode::Decode( AVMediaType _type )
    :   type(_type)
{}






/*******************************************************************************
Decode::~Decode()
********************************************************************************/
Decode::~Decode()
{}




/*******************************************************************************
Decode::get_frame_count()
********************************************************************************/
int Decode::get_frame_count()
{
    return  frame_count;
}



/*******************************************************************************
Decode::init()
********************************************************************************/
int     Decode::init()
{
    int     ret     =   0;

    frame_count =   -1;
    frame       =   av_frame_alloc();
    if( frame == nullptr ) 
    {
        MYLOG( LOG::L_ERROR, "Could not allocate frame" );
        ret =   AVERROR(ENOMEM);        
        return  ret;
    }

    return  R_SUCCESS;
}





/*******************************************************************************
Decode::get_stream()
********************************************************************************/
AVStream*   Decode::get_stream()
{
    if( stream == nullptr )
        MYLOG( LOG::L_ERROR, "stream is null." );
    return  stream;
}





/*******************************************************************************
Decode::open_codec_context()
********************************************************************************/
int     Decode::open_codec_context( int stream_index, AVFormatContext *fmt_ctx, AVMediaType type )
{
    int         ret     =   0;
    const AVCodec     *dec    =   nullptr;

    //
    stream  =   fmt_ctx->streams[stream_index];

    // find decoder for the stream 
    dec     =   avcodec_find_decoder( stream->codecpar->codec_id );
    if( dec == nullptr )
    {
        auto str =  av_get_media_type_string(type);
        MYLOG( LOG::L_ERROR, "Failed to find %s codec. error code = %d", str, AVERROR(EINVAL) );
        return  R_ERROR;
    }

    // Allocate a codec context for the decoder 
    dec_ctx    =   avcodec_alloc_context3(dec);
    if( dec_ctx == nullptr )
    {
        auto str    =   av_get_media_type_string(type);
        MYLOG( LOG::L_ERROR, "Failed to allocate the %s codec context. error code = %d", str,  AVERROR(ENOMEM) );
        return  R_ERROR;
    }

    // Copy codec parameters from input stream to output codec context 
    ret =   avcodec_parameters_to_context( dec_ctx, stream->codecpar );
    if( ret < 0 )
    {
        auto str    =   av_get_media_type_string(type);
        MYLOG( LOG::L_ERROR, "Failed to copy %s codec parameters to decoder context. ret = %d", str, ret );
        return  R_ERROR;
    }

    // Init the decoders 
    ret =   avcodec_open2( dec_ctx, dec, nullptr );
    if( ret < 0 )
    {
        auto str    =   av_get_media_type_string(type);
        MYLOG( LOG::L_ERROR, "Failed to open %s codec. ret = %d", str, ret );
        return  R_ERROR;
    }   

    // for psg subtitle use.
    // 沒設置的話, decode psg subtitle 的時候無法取得timestamp.
    dec_ctx->pkt_timebase   =   fmt_ctx->streams[stream_index]->time_base;

    // output info
    output_decode_info( (AVCodec*)dec, dec_ctx );

    return  R_SUCCESS;
}






/*******************************************************************************
Decode::send_packet()
********************************************************************************/
int     Decode::send_packet( AVPacket *pkt )
{
    int     ret =   0;
    char    buf[AV_ERROR_MAX_STRING_SIZE]{0};

    // submit the packet to the decoder
    ret =   avcodec_send_packet( dec_ctx, pkt );
    if( ret < 0 ) 
    {
        auto str    =   av_make_error_string( buf, AV_ERROR_MAX_STRING_SIZE, ret );
        MYLOG( LOG::L_WARN, "Error submitting a packet for decoding (%s)", str );
        return  ret;
    }

    return  R_SUCCESS;
}





/*******************************************************************************
Decode::get_frame()
********************************************************************************/
AVFrame*    Decode::get_frame()
{
    return  frame;
}





/*******************************************************************************
Decode::get_decode_context_type()

if( get_decode_context_type() == AVMEDIA_TYPE_VIDEO )
{}

可以用上面的範例,從decoder判斷屬於video還是audio.
********************************************************************************/
AVMediaType     Decode::get_decode_context_type()
{
    if( dec_ctx == nullptr )
        return  AVMEDIA_TYPE_SUBTITLE;
    else
        return  dec_ctx->codec->type;
}





/*******************************************************************************
Decode::flush_for_seek()
********************************************************************************/
void    Decode::flush_for_seek()
{
    avcodec_flush_buffers( dec_ctx );
}






/*******************************************************************************
Decode::recv_frame()
********************************************************************************/
int     Decode::recv_frame( int index )
{
    char    buf[AV_ERROR_MAX_STRING_SIZE]{0};
    int     ret     =   0;

    ret     =   avcodec_receive_frame( dec_ctx, frame );
    if( ret < 0 ) 
    {
        // those two return values are special and mean there is no output
        // frame available, but there were no errors during decoding
        if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN) )
            return  0;

        auto str    =   av_make_error_string( buf, AV_ERROR_MAX_STRING_SIZE, ret );
        MYLOG( LOG::L_ERROR, "Error during decoding (%s)", str );
        return  ret;
    }

    frame_count++;       // 這個參數錯掉會影響後續播放.
    return  HAVE_FRAME;  // 表示有 frame 被解出來. 
}








/*******************************************************************************
Decode::unref_frame()
********************************************************************************/
void     Decode::unref_frame()
{
    av_frame_unref(frame);
}





/*******************************************************************************
Decode::end()
********************************************************************************/
int     Decode::end()
{
    av_frame_free( &frame );
    frame   =   nullptr;

    avcodec_free_context( &dec_ctx );
    dec_ctx     =   nullptr;
    stream      =   nullptr;

    //
    frame_count =   -1;
    is_current  =   false;

    return  R_SUCCESS;
}





#ifdef FFMPEG_TEST
/*******************************************************************************
Decode::flush()

flush 過程基本上同 decode, 送 nullptr 進去
也會吐出一張 frame, 需要將此 frame 資料寫入 output.
********************************************************************************/
int    Decode::flush()
{
    int     ret =   0;
    char    buf[AV_ERROR_MAX_STRING_SIZE]{0};

    // submit the packet to the decoder
    ret =   avcodec_send_packet( dec_ctx, nullptr );
    if( ret < 0 ) 
    {
        auto str    =   av_make_error_string( buf, AV_ERROR_MAX_STRING_SIZE, ret );
        MYLOG( LOG::L_ERROR, "Error submitting a packet for decoding (%s)", str );
        return  R_ERROR;
    }
    
    // get all the available frames from the decoder
    while( ret >= 0 )
    {
        ret =   avcodec_receive_frame( dec_ctx, frame );
        if( ret < 0 ) 
        {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN) )
                break; 
    
            auto str    =   av_make_error_string( buf, AV_ERROR_MAX_STRING_SIZE, ret );
            MYLOG( LOG::L_ERROR, "Error during decoding (%s)", str );
            break; //return  ret;
        }
    
        // write the frame data to output file
        if( is_current == true )
        {
            frame_count++;
            output_frame_func();
        }
    
        av_frame_unref(frame);
    
        if( ret < 0 )
            break;
    }

    return 0;
}
#endif




/*******************************************************************************
Decode::get_decode_context()
********************************************************************************/
AVCodecContext*  Decode::get_decode_context()
{
    return  dec_ctx;
}




/*******************************************************************************
Decode::set_is_current()
********************************************************************************/
void    Decode::set_is_current( bool flag )
{
    is_current  =   flag;
}




/*******************************************************************************
Decode::get_is_current()
********************************************************************************/
bool    Decode::get_is_current()
{
    return  is_current;
}
