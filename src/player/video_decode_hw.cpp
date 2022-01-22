#include "video_decode_hw.h"

#include "../hw/NvDecoder.h"
#include "sub_decode.h"

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
AV_PIX_FMT_NV12
********************************************************************************/
int     VideoDecodeHW::init()
{
    pkt_bsf     =   av_packet_alloc();

    width       =   dec_ctx->width;
    height      =   dec_ctx->height;

    // init nvidia decoder.
    init_nvidia_decoder();

    MYLOG( LOG::INFO, "width = %d, height = %d, pix_fmt = %d\n", width, height, pix_fmt );
    
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_RGB24, 1 );
    //video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_YUV420P, 1 );

    if( video_dst_bufsize < 0 )
    {
        MYLOG( LOG::ERROR, "Could not allocate raw video buffer" );
        return  ERROR;
    }

    //
    AVPixelFormat  tmp_fmt  =   get_pix_fmt();

    sws_ctx     =   sws_getContext( width, height, tmp_fmt,                     // src
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





inline cudaVideoCodec FFmpeg2NvCodecId(AVCodecID id) {
    switch (id) {
    case AV_CODEC_ID_MPEG1VIDEO : return cudaVideoCodec_MPEG1;
    case AV_CODEC_ID_MPEG2VIDEO : return cudaVideoCodec_MPEG2;
    case AV_CODEC_ID_MPEG4      : return cudaVideoCodec_MPEG4;
    case AV_CODEC_ID_VC1        : return cudaVideoCodec_VC1;
    case AV_CODEC_ID_H264       : return cudaVideoCodec_H264;
    case AV_CODEC_ID_HEVC       : return cudaVideoCodec_HEVC;
    case AV_CODEC_ID_VP8        : return cudaVideoCodec_VP8;
    case AV_CODEC_ID_VP9        : return cudaVideoCodec_VP9;
    case AV_CODEC_ID_MJPEG      : return cudaVideoCodec_JPEG;
    default                     : return cudaVideoCodec_NumCodecs;
    }
}



/*******************************************************************************
VideoDecodeHW::init_nvidia_decoder
********************************************************************************/
int     VideoDecodeHW::init_nvidia_decoder()
{
    AVCodecID   codec_id    =   dec_ctx->codec_id;
    cudaVideoCodec cuda_codec = FFmpeg2NvCodecId(codec_id);


    cuInit(0);
    int     gpu_index   =   0; // 使用第一張顯示卡
    int     gpu_num     =   0;
    cuDeviceGetCount( &gpu_num );
    if( gpu_index >= gpu_num ) 
    {
        MYLOG( LOG::ERROR, "no video card." );
        return  ERROR;
    }

    CUdevice    nv_dev  =   0;
    cuDeviceGet( &nv_dev, gpu_index );

    char    device_name[80];
    cuDeviceGetName( device_name, sizeof(device_name), nv_dev );
    MYLOG( LOG::INFO, "gpu use %s", device_name )

    CUcontext   cuda_ctx    =   nullptr;
    cuCtxCreate( &cuda_ctx, 0, nv_dev );

    //
    Rect    rRect   =   {};
    Dim     resize_dim = {};
    nv_decoder  =   new NvDecoder( cuda_ctx, width, height, false, cuda_codec, nullptr, false, false, &rRect, &resize_dim );

    return  SUCCESS;
}



/*******************************************************************************
VideoDecodeHW::get_pix_fmt
********************************************************************************/
AVPixelFormat   VideoDecodeHW::get_pix_fmt()
{
    if( stream->codecpar->format == AV_PIX_FMT_YUV420P )
        return  AV_PIX_FMT_YUV420P;
    else
        return  AV_PIX_FMT_YUV420P16LE;

}



/*******************************************************************************
VideoDecodeHW::init_bsf
********************************************************************************/
int     VideoDecodeHW::init_bsf( AVFormatContext *fmt_ctx )
{
    AVCodecID   codec_id    =   dec_ctx->codec_id;
    int     ret     =   0;

    if( use_bsf == true )
    {
        const AVBitStreamFilter*    bsf     =   nullptr;

        if( codec_id == AV_CODEC_ID_H264 )
            bsf     =   av_bsf_get_by_name("h264_mp4toannexb");
        else if( codec_id == AV_CODEC_ID_HEVC )
            bsf     =   av_bsf_get_by_name("hevc_mp4toannexb");
        else 
            assert(0);

        ret     =   av_bsf_alloc( bsf, &v_bsf_ctx );
        assert( ret == 0 );

        v_bsf_ctx->par_in   =   fmt_ctx->streams[cs_index]->codecpar;
        av_bsf_init( v_bsf_ctx );
    }

    return  SUCCESS;
}






/*******************************************************************************
VideoDecodeHW::end
********************************************************************************/
int     VideoDecodeHW::end()
{
    VideoDecode::end();

    return  SUCCESS;
}





/*******************************************************************************
VideoDecodeHW::send_packet
********************************************************************************/
int     VideoDecodeHW::send_packet( AVPacket *pkt )
{
    if( pkt_bsf->data ) 
        av_packet_unref(pkt_bsf);

    if( use_bsf == true )
    {
        av_bsf_send_packet( v_bsf_ctx, pkt );
        //printf( "pts = %lld\n", pkt_bsf->pts) ;


        av_bsf_receive_packet( v_bsf_ctx, pkt_bsf );
      

        //printf( "pkt_bsf = %lld\n", pkt_bsf->pts) ;

        // insert nv decode here.
        uint8_t *ppVideo = pkt_bsf->data;
        int pnVideoBytes = pkt_bsf->size;

        static int n = 0;

        nv_decoder->Decode( ppVideo, pnVideoBytes, &ppFrame, &nFrameReturned, CUVID_PKT_ENDOFPICTURE, &p_timestamp, pkt_bsf->pts );
        //MYLOG( LOG::DEBUG, "nFrameReturned = %d", nFrameReturned );
        recv_count  =   0;
    }

    return  SUCCESS;
}



#include "../hw/NvCodecUtils.h"



void ConvertToPlanar(uint8_t *pHostFrame, int nWidth, int nHeight, int nBitDepth) 
{
    if (nBitDepth == 8) 
    {
        // nv12->iyuv
        YuvConverter<uint8_t> converter8(nWidth, nHeight);
        converter8.UVInterleavedToPlanar(pHostFrame);
    } 
    else 
    {
        // p016->yuv420p16
        YuvConverter<uint16_t> converter16(nWidth, nHeight);
        converter16.UVInterleavedToPlanar((uint16_t *)pHostFrame);
    }
}




/*******************************************************************************
VideoDecodeHW::recv_frame
********************************************************************************/
int     VideoDecodeHW::recv_frame( int index )
{
    if( nFrameReturned == 0 || recv_count == nFrameReturned )
        return  0;

    if( use_bsf == true )
    {     
        ConvertToPlanar( ppFrame[recv_count], nv_decoder->GetWidth(), nv_decoder->GetHeight(), nv_decoder->GetBitDepth());        

        frame->format   =   get_pix_fmt();
        frame->width    =   width;
        frame->height   =   height;

        int ret     =   av_frame_get_buffer( frame, 0 );
        if( ret < 0 ) 
            MYLOG( LOG::ERROR, "get buffer fail." );

        ret     =   av_frame_make_writable(frame);
        if( ret < 0 )
            assert(0);

        int fsize = nv_decoder->GetFrameSize();
        
        /*memcpy( frame->data[0], ppFrame[recv_count], 1280*720 );
        memcpy( frame->data[1] , ppFrame[recv_count] + width*height , 1280*720/4 );
        memcpy( frame->data[2] , ppFrame[recv_count] + width*height*5/4 , 1280*720/4 );*/

        AVPixelFormat  tmp_fmt  =   get_pix_fmt();
        av_image_fill_arrays( frame->data, frame->linesize, ppFrame[recv_count], tmp_fmt, width, height, 1 );







        frame->best_effort_timestamp = p_timestamp[recv_count];
        frame->pts = frame->best_effort_timestamp;

        frame_pts   =   frame->best_effort_timestamp;
        

        //av_image_copy( frame->data, frame->linesize, (const uint8_t**)ppFrame[recv_count], frame->linesize, AV_PIX_FMT_YUV420P16LE, width, height );


        // need add time stamp here.

        recv_count++;
        frame_count++;


        // exist frame.
        //if( ret > 0 )
        //{
            //frame_pts   =   frame->best_effort_timestamp;

        if( sub_dec != nullptr )
        {
            if( sub_dec->is_graphic_subtitle() == true )
                ret     =   overlap_subtitle_image();
            else
                ret     =   render_nongraphic_subtitle();
        }
        //}

    }

    if( (recv_count-1) < nFrameReturned )
        return  1;
    else
        return  0;
}




/*******************************************************************************
VideoDecodeHW::output_video_data
********************************************************************************/
VideoData   VideoDecodeHW::output_video_data()
{
    VideoData   vd;
    vd.index        =   frame_count;
    vd.timestamp    =   get_timestamp();

    if( sub_dec == nullptr )        
        vd.frame        =   get_video_image();
    else
    {
        if( sub_dec->is_graphic_subtitle() == false )        
            vd.frame    =   sub_dec->get_subtitle_image();        
        else
            vd.frame    =   overlay_image;
    }

    return  vd;
}
