#include "decode.h"
#include "tool.h"

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
Decode::Decode()
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

    frame_count =   0;
    frame       =   av_frame_alloc();
    if( frame == nullptr ) 
    {
        MYLOG( LOG::ERROR, "Could not allocate frame" );
        ret =   AVERROR(ENOMEM);        
        return  ret;
    }

    return  SUCCESS;
}






/*******************************************************************************
Decode::open_codec_context()
********************************************************************************/
int     Decode::open_codec_context( int stream_index, AVFormatContext *fmt_ctx, AVMediaType type )
{
    int         ret =   0;
    AVStream    *st =   nullptr;
    AVCodec     *dec    =   nullptr;

    //
    st  =   fmt_ctx->streams[stream_index];
    stream = fmt_ctx->streams[stream_index];

    // find decoder for the stream 
    dec =   avcodec_find_decoder( st->codecpar->codec_id );
    if( dec == nullptr )
    {
        auto str =  av_get_media_type_string(type);
        MYLOG( LOG::ERROR, "Failed to find %s codec. error code = %d", str, AVERROR(EINVAL) );
        return  ERROR;
    }

    // Allocate a codec context for the decoder 
    dec_ctx    =   avcodec_alloc_context3(dec);
    if( dec_ctx == nullptr )
    {
        auto str    =   av_get_media_type_string(type);
        MYLOG( LOG::ERROR, "Failed to allocate the %s codec context. error code = %d", str,  AVERROR(ENOMEM) );
        return  ERROR;
    }

    // Copy codec parameters from input stream to output codec context 
    ret =   avcodec_parameters_to_context( dec_ctx, st->codecpar );
    if( ret < 0 )
    {
        auto str    =   av_get_media_type_string(type);
        MYLOG( LOG::ERROR, "Failed to copy %s codec parameters to decoder context. ret = %d", str, ret );
        return  ERROR;
    }

    // Init the decoders 
    ret =   avcodec_open2( dec_ctx, dec, nullptr );
    if( ret < 0 )
    {
        auto str    =   av_get_media_type_string(type);
        MYLOG( LOG::ERROR, "Failed to open %s codec. ret = %d", str, ret );
        return  ERROR;
    }   

    // output info
    output_decode_info( dec, dec_ctx );

    return  SUCCESS;
}






/*******************************************************************************
Decode::send_packet()
********************************************************************************/
int     Decode::send_packet( const AVPacket *pkt )
{
    int     ret =   0;
    char    buf[AV_ERROR_MAX_STRING_SIZE]{0};

    // submit the packet to the decoder
    ret =   avcodec_send_packet( dec_ctx, pkt );
    if( ret < 0 ) 
    {
        auto str    =   av_make_error_string( buf, AV_ERROR_MAX_STRING_SIZE, ret );
        MYLOG( LOG::WARN, "Error submitting a packet for decoding (%s)", str );
        return  ret;
    }

    return  SUCCESS;
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
    return  dec_ctx->codec->type;
}




/*******************************************************************************
Decode::recv_frame()
********************************************************************************/
int     Decode::recv_frame()
{
    char    buf[AV_ERROR_MAX_STRING_SIZE]{0};
    int     ret     =   0;

    ret     =   avcodec_receive_frame( dec_ctx, frame );
    if( ret < 0 ) 
    {
        // those two return values are special and mean there is no output
        // frame available, but there were no errors during decoding
        if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN) )
            return 0;

        auto str    =   av_make_error_string( buf, AV_ERROR_MAX_STRING_SIZE, ret );
        MYLOG( LOG::ERROR, "Error during decoding (%s)", str );
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
    avcodec_free_context( &dec_ctx );
    av_frame_free( &frame );

    return  SUCCESS;
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
        MYLOG( LOG::ERROR, "Error submitting a packet for decoding (%s)", str );
        return  ERROR;
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
                break; //return 0;

            auto str    =   av_make_error_string( buf, AV_ERROR_MAX_STRING_SIZE, ret );
            MYLOG( LOG::ERROR, "Error during decoding (%s)", str );
            break; //return  ret;
        }

        // write the frame data to output file
        output_frame_func();
        av_frame_unref(frame);

        if( ret < 0 )
            break; //return ret;
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
