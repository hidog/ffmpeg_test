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
Decode::open_all_codec()
********************************************************************************/
int     Decode::open_all_codec( AVFormatContext *fmt_ctx, AVMediaType type )
{
    int         ret     =   0;
    int         index;
    AVStream    *st     =   nullptr;
    AVCodec     *dec    =   nullptr;

    cs_index  =   -1;

    //
    for( index = 0; index < fmt_ctx->nb_streams; index++ )
    {
        ret  =   av_find_best_stream( fmt_ctx, type, index, -1, NULL, 0 );
        if( ret >= 0 )
        {
            if( cs_index < 0 )           
                cs_index   =   index; // choose first ad current.

            // note: dec_ctx, stream is class member. after open codec, they use for current ctx, stream.
            open_codec_context( index, fmt_ctx, type );            
            dec_map.emplace(    std::make_pair(index,dec_ctx) ); 
            stream_map.emplace( std::make_pair(index,stream)  );
        }
    }

    // set
    dec_ctx     =   dec_map[cs_index];
    stream      =   stream_map[cs_index];

    return  SUCCESS;
}





/*******************************************************************************
Decode::exist_stream()
********************************************************************************/
bool    Decode::exist_stream()
{
    return  cs_index >= 0 ? true : false;
}



/*******************************************************************************
Decode::current_index()
********************************************************************************/
int     Decode::current_index()
{
    return  cs_index;
}




/*******************************************************************************
Decode::is_index()
********************************************************************************/
bool    Decode::find_index( int index )
{
    auto    item    =   dec_map.find(index);
    return  item != dec_map.end();
}






/*******************************************************************************
Decode::open_codec_context()
********************************************************************************/
int     Decode::open_codec_context( int stream_index, AVFormatContext *fmt_ctx, AVMediaType type )
{
    int         ret     =   0;
    AVCodec     *dec    =   nullptr;

    //
    stream  =   fmt_ctx->streams[stream_index];

    // find decoder for the stream 
    dec     =   avcodec_find_decoder( stream->codecpar->codec_id );
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
    ret =   avcodec_parameters_to_context( dec_ctx, stream->codecpar );
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
    int     index   =   pkt == nullptr ? cs_index : pkt->stream_index;

    int     ret =   0;
    char    buf[AV_ERROR_MAX_STRING_SIZE]{0};

    // submit the packet to the decoder
    AVCodecContext  *ctx    =   dec_map[index];
    ret =   avcodec_send_packet( ctx, pkt );
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
int     Decode::recv_frame( int index )
{
    if( index == -1 ) // -1 use for flush.
        index   =   cs_index; 

    char    buf[AV_ERROR_MAX_STRING_SIZE]{0};
    int     ret     =   0;

    AVCodecContext  *dec    =   dec_map[index];
    ret     =   avcodec_receive_frame( dec, frame );
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
    //avcodec_free_context( &dec_ctx );

    av_frame_free( &frame );
    frame   =   nullptr;

    //
    for( auto itr : dec_map )    
        avcodec_free_context( &itr.second );
    dec_map.clear();
    dec_ctx     =   nullptr;

    //
    stream_map.clear();
    stream      =   nullptr;

    //
    cs_index    =   -1;

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
