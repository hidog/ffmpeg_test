#include "video_decode_hw.h"

#include "../hw/NvDecoder.h"
#include "../hw/NvCodecUtils.h"

#include "sub_decode.h"
#include "../imgprcs/image_process.h"



extern "C" {

#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

} // end extern "C"







/*******************************************************************************
VideoDecodeHW::VideoDecodeHW()
********************************************************************************/
VideoDecodeHW::VideoDecodeHW()
    :   VideoDecode()
{}








/*******************************************************************************
VideoDecodeHW::~VideoDecodeHW()
********************************************************************************/
VideoDecodeHW::~VideoDecodeHW()
{
    end();
}





/*******************************************************************************
VideoDecodeHW::list_hw_decoders
********************************************************************************/
void    VideoDecodeHW::list_hw_decoders()
{
    AVHWDeviceType  hw_type     =   AV_HWDEVICE_TYPE_NONE;

    while( true )
    {
        hw_type     =   av_hwdevice_iterate_types( hw_type );
        if( hw_type == AV_HWDEVICE_TYPE_NONE )
            break;
        MYLOG( LOG::L_INFO, "hw device type = %s", av_hwdevice_get_type_name(hw_type) );
    }
}





/*******************************************************************************
VideoDecodeHW::open_codec_context
********************************************************************************/
int     VideoDecodeHW::open_codec_context( AVFormatContext *fmt_ctx )
{
    int     ret     =   VideoDecode::open_codec_context( fmt_ctx );
    return  ret;
}




/*******************************************************************************
VideoDecodeHW::init
********************************************************************************/
int     VideoDecodeHW::init()
{
    list_hw_decoders();

    assert( stream != nullptr );
    width       =   stream->codecpar->width;
    height      =   stream->codecpar->height;

    MYLOG( LOG::L_INFO, "width = %d, height = %d\n", width, height );
    
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_RGB24, 1 );
    //video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_YUV420P, 1 );  use for opencv

    if( video_dst_bufsize < 0 )
    {
        MYLOG( LOG::L_ERROR, "Could not allocate raw video buffer" );
        return  R_ERROR;
    }

    //
    AVPixelFormat  nv_pix_fmt   =   get_pix_fmt();

    sws_ctx     =   sws_getContext( width, height, nv_pix_fmt,                  // src
                                    width, height, AV_PIX_FMT_RGB24,            // dst
                                    SWS_BICUBIC, NULL, NULL, NULL);                        

#ifdef FFMPEG_TEST
    output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_QT, this );
    //output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_openCV, this );
#endif

    Decode::init();

    // 理論上可以在這之前就設置好 sub_dec, 但目前規劃是 init 後再設置 sub_dec.
    if( sub_dec != nullptr )
        MYLOG( LOG::L_ERROR, "sub_dec not null." );

    return  R_SUCCESS;
}




/*******************************************************************************
VideoDecodeHW::end
********************************************************************************/
int     VideoDecodeHW::end()
{
    VideoDecode::end();
    return  R_SUCCESS;
}





/*******************************************************************************
VideoDecodeHW::send_packet
********************************************************************************/
int     VideoDecodeHW::send_packet( AVPacket *pkt )
{
    assert(0);
    return  R_SUCCESS;
}




/*******************************************************************************
VideoDecodeHW::flush_for_seek
********************************************************************************/
void    VideoDecodeHW::flush_for_seek()
{
    assert(0);
}




/*******************************************************************************
VideoDecodeHW::recv_frame
********************************************************************************/
int     VideoDecodeHW::recv_frame( int index )
{
    assert(0);
    return  0;
}






#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecodeHW::flush
********************************************************************************/
int     VideoDecodeHW::flush()
{
    int     ret =   0;
    char    buf[AV_ERROR_MAX_STRING_SIZE]{0};


    // submit the packet to the decoder
    ret =   send_packet( nullptr );
    if( ret < 0 ) 
    {
        MYLOG( LOG::L_ERROR, "flush error" );
        return  R_ERROR;
    }
    
    // get all the available frames from the decoder
    while( ret >= 0 )
    {
        ret =   recv_frame( -1 );  // flush 階段必須傳入 < 0 的值
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
        output_frame_func();     
        av_frame_unref(frame);
    }
    

    return 0;
}
#endif


