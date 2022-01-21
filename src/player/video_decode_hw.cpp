#include "video_decode_hw.h"


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
{}





/*******************************************************************************
VideoDecodeHW::open_codec_context
********************************************************************************/
int     VideoDecodeHW::open_codec_context( AVFormatContext *fmt_ctx )
{
    int     ret     =   VideoDecode::open_codec_context( fmt_ctx );

    AVCodecID       codec_id    =   dec_ctx->codec_id;
    std::string     log_name    =   fmt_ctx->iformat->long_name;

    bool    flag1   =   log_name == "QuickTime / MOV"    ||
                        log_name == "FLV (Flash Video)"  ||
                        log_name == "Matroska / WebM" ;

    if( flag1 == false )
        printf("test");

    bool    flag2   =   codec_id == AV_CODEC_ID_H264 || codec_id == AV_CODEC_ID_HEVC;

    use_bsf     =   flag1 && flag2;

    if( use_bsf == false )
        printf("test");

    // NOTE: hardware decode need init bsf.
    init_bsf(fmt_ctx);

    return  ret;
}




/*******************************************************************************
VideoDecodeHW::init

ffplay -f rawvideo -pix_fmt nv12 -video_size 1280x720 output.data
ffplay -f rawvideo -pix_fmt p016 -video_size 1920x1080 2output.data

AV_PIX_FMT_P016LE
********************************************************************************/
int     VideoDecodeHW::init()
{
    int ret     =   0;
    width       =   dec_ctx->width;
    height      =   dec_ctx->height;

    MYLOG( LOG::INFO, "width = %d, height = %d, pix_fmt = %d\n", width, height, pix_fmt );
    
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_RGB24, 1 );
    if( video_dst_bufsize < 0 )
    {
        MYLOG( LOG::ERROR, "Could not allocate raw video buffer" );
        return  ERROR;
    }

    // NOTE : 可以改變寬高. 
    sws_ctx     =   sws_getContext( width, height, AV_PIX_FMT_P016LE,                     // src
                                    width, height, AV_PIX_FMT_RGB24,            // dst
                                    SWS_BICUBIC, NULL, NULL, NULL);                        

#ifdef FFMPEG_TEST
    output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_QT, this );
    //output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_openCV, this );
#endif

    Decode::init();

    // 理論上可以在這之前就設置好 sub_dec, 但目前規劃是 init 後再設置 sub_dec.
    if( sub_dec != nullptr )
        MYLOG( LOG::ERROR, "sub_dec not null." );

    return  SUCCESS;
}




/*******************************************************************************
VideoDecodeHW::init_bsf
********************************************************************************/
int     VideoDecodeHW::init_bsf( AVFormatContext *fmt_ctx )
{
    AVCodecID   codec_id    =   dec_ctx->codec_id;


    if( use_bsf == true )
    {
        const AVBitStreamFilter*    bsf     =   nullptr;

        if( codec_id == AV_CODEC_ID_H264 )
            bsf     =   av_bsf_get_by_name("h264_mp4toannexb");
        else if( codec_id == AV_CODEC_ID_HEVC )
            bsf     =   av_bsf_get_by_name("hevc_mp4toannexb");
        else 
            assert(0);

        av_bsf_alloc( bsf, &v_bsf_ctx );
        v_bsf_ctx->par_in   =   fmt_ctx->streams[cs_index]->codecpar;
        av_bsf_init( v_bsf_ctx );
    }
}






/*******************************************************************************
VideoDecodeHW::end
********************************************************************************/
int     VideoDecodeHW::end()
{
    VideoDecode::end();
}





/*******************************************************************************
VideoDecodeHW::send_packet
********************************************************************************/
int     VideoDecodeHW::send_packet( AVPacket *pkt )
{
    if( use_bsf == true )
    {
        av_bsf_send_packet( v_bsf_ctx, pkt );
        av_bsf_receive_packet( v_bsf_ctx, pkt_bsf );
      
        // insert nv decode here.
    }
}
