#include "video_decode_hw.h"

#include "../hw/NvDecoder.h"
#include "../hw/NvCodecUtils.h"

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
{
    end();
}





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

    bool    flag2   =   codec_id == AV_CODEC_ID_H264 || codec_id == AV_CODEC_ID_HEVC;

    use_bsf     =   flag1 && flag2;

    if( use_bsf == false )
        MYLOG( LOG::ERROR, "not support nv decode.");  // 測試一下不支援的影片會發生甚麼事情

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

    MYLOG( LOG::INFO, "width = %d, height = %d\n", width, height );
    
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_RGB24, 1 );
    //video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_YUV420P, 1 );  use for opencv

    if( video_dst_bufsize < 0 )
    {
        MYLOG( LOG::ERROR, "Could not allocate raw video buffer" );
        return  ERROR;
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
        MYLOG( LOG::ERROR, "sub_dec not null." );

    return  SUCCESS;
}





/*******************************************************************************
VideoDecodeHW::codec_id_ffmpeg_to_cuda
********************************************************************************/
cudaVideoCodec  VideoDecodeHW::codec_id_ffmpeg_to_cuda(AVCodecID id) 
{
    switch (id) 
    {
    case AV_CODEC_ID_MPEG1VIDEO : 
        return cudaVideoCodec_MPEG1;
    case AV_CODEC_ID_MPEG2VIDEO : 
        return cudaVideoCodec_MPEG2;
    case AV_CODEC_ID_MPEG4      : 
        return cudaVideoCodec_MPEG4;
    case AV_CODEC_ID_VC1        : 
        return cudaVideoCodec_VC1;
    case AV_CODEC_ID_H264       : 
        return cudaVideoCodec_H264;
    case AV_CODEC_ID_HEVC       : 
        return cudaVideoCodec_HEVC;
    case AV_CODEC_ID_VP8        : 
        return cudaVideoCodec_VP8;
    case AV_CODEC_ID_VP9        : 
        return cudaVideoCodec_VP9;
    case AV_CODEC_ID_MJPEG      : 
        return cudaVideoCodec_JPEG;
    default                     : 
        return cudaVideoCodec_NumCodecs;
    }
}



/*******************************************************************************
VideoDecodeHW::init_nvidia_decoder
********************************************************************************/
int     VideoDecodeHW::init_nvidia_decoder()
{
    AVCodecID       codec_id    =   stream->codecpar->codec_id;  //  dec_ctx->codec_id;
    cudaVideoCodec  cuda_codec  =   codec_id_ffmpeg_to_cuda(codec_id);

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
    Rect    rect        =   {};
    Dim     resize_dim  =   {};
    nv_decoder  =   new NvDecoder( cuda_ctx, width, height, false, cuda_codec, nullptr, false, false, &rect, &resize_dim );

    return  SUCCESS;
}



/*******************************************************************************
VideoDecodeHW::get_pix_fmt
********************************************************************************/
AVPixelFormat   VideoDecodeHW::get_pix_fmt()
{
    if( stream->codecpar->format == AV_PIX_FMT_YUV420P )
        return  AV_PIX_FMT_YUV420P;
    else if( stream->codecpar->format == AV_PIX_FMT_YUV420P10LE )
        return  AV_PIX_FMT_YUV420P16LE;
    else
    {
        // note: 理論上能相容 測試看看
        MYLOG( LOG::ERROR, "not yuv420p, yuv420p10" );
        return  AV_PIX_FMT_YUV420P16LE;    
    }
}



/*******************************************************************************
VideoDecodeHW::init_bsf
********************************************************************************/
int     VideoDecodeHW::init_bsf( AVFormatContext* fmt_ctx )
{
    AVCodecID   codec_id    =   stream->codecpar->codec_id;
    int         ret         =   0;

    if( use_bsf == true )
    {
        const AVBitStreamFilter     *bsf    =   nullptr;

        if( codec_id == AV_CODEC_ID_H264 )
            bsf     =   av_bsf_get_by_name("h264_mp4toannexb");
        else if( codec_id == AV_CODEC_ID_HEVC )
            bsf     =   av_bsf_get_by_name("hevc_mp4toannexb");
        else 
        {
            MYLOG( LOG::ERROR, "un support." );
        }

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

    if( nv_decoder != nullptr )
    {
        delete nv_decoder;
        nv_decoder  =   nullptr;
    }

    if( pkt_bsf != nullptr )
    {
        av_packet_free( &pkt_bsf );
        pkt_bsf     =   nullptr;
    }
 
    if( v_bsf_ctx != nullptr )
    {
        av_bsf_free( &v_bsf_ctx );
        v_bsf_ctx   =   nullptr;
    }
 
    //
    use_bsf     =   false;  
    recv_size   =   0;
    recv_count  =   0; 
    p_timestamp =   nullptr;

    return  SUCCESS;
}





/*******************************************************************************
VideoDecodeHW::send_packet
********************************************************************************/
int     VideoDecodeHW::send_packet( AVPacket *pkt )
{
    uint8_t     *pkt_ptr    =   nullptr;
    int         pkt_size    =   0;
    int64_t     pkt_ts      =   0;

    if( use_bsf == true )
    {
        av_bsf_send_packet( v_bsf_ctx, pkt );
        av_bsf_receive_packet( v_bsf_ctx, pkt_bsf );

        pkt_ptr     =   pkt_bsf->data;
        pkt_size    =   pkt_bsf->size;
    }
    else
    {
        pkt_ptr     =   pkt->data;
        pkt_size    =   pkt->size;
    }

    // decode
    nv_decoder->Decode( pkt_ptr, pkt_size, &nv_frames, &recv_size, CUVID_PKT_ENDOFPICTURE, &p_timestamp, pkt_bsf->pts );

    // need set count to zero. and use in recv loop.
    recv_count  =   0;    

    return  SUCCESS;
}




/*******************************************************************************
VideoDecodeHW::flush_for_seek
********************************************************************************/
void    VideoDecodeHW::flush_for_seek()
{
    av_bsf_flush( v_bsf_ctx );
    Decode::flush_for_seek();
}





/*******************************************************************************
VideoDecodeHW::convert_to_planar
********************************************************************************/
void    VideoDecodeHW::convert_to_planar( uint8_t *ptr, int w, int h, int d )
{
    if( d == 8 )
    {
        // nv12->iyuv
        YuvConverter<uint8_t>   converter8( w, h );
        converter8.UVInterleavedToPlanar( ptr );
    } 
    else 
    {
        // p016->yuv420p16
        YuvConverter<uint16_t>  converter16( w, h );
        converter16.UVInterleavedToPlanar( (uint16_t *)ptr );
    }
}






/*******************************************************************************
VideoDecodeHW::recv_frame
********************************************************************************/
int     VideoDecodeHW::recv_frame( int index )
{
    // note: 理論上 video 只有一個 stream, 這邊就不處理 index 了.

    if( recv_size == 0 || recv_count == recv_size )
        return  0;  // this loop has no frame.

    if( use_bsf == false )
        MYLOG( LOG::ERROR, "use bsf is false." )  // 測試看看 use_bsf == false 的時候程式是否能運行
      
    // 不轉換到 plannar, 則要改 sws 的 pix_fmt.
    int     w   =   nv_decoder->GetWidth();
    int     h   =   nv_decoder->GetHeight();
    int     d   =   nv_decoder->GetBitDepth();
    convert_to_planar( nv_frames[recv_count], w, h, d );
    
    frame->format   =   get_pix_fmt();
    frame->width    =   width;
    frame->height   =   height;
    
    // allocate frame data.
    int ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "get buffer fail." );
    
    ret     =   av_frame_make_writable(frame);
    if( ret < 0 )
        assert(0);
    
    //int fsize = nv_decoder->GetFrameSize();
    
    // copy to frame
    AVPixelFormat   nv_fmt  =   get_pix_fmt();
    av_image_fill_arrays( frame->data, frame->linesize, nv_frames[recv_count], nv_fmt, width, height, 1 );
    
    // time stamp.
    frame->best_effort_timestamp    =   p_timestamp[recv_count];
    frame->pts                      =   frame->best_effort_timestamp;
    frame_pts                       =   frame->best_effort_timestamp;       
    
    recv_count++;   // use for control loop.
    frame_count++;
    
    // render subtitle if exist.    
    if( sub_dec != nullptr )
    {
        if( sub_dec->is_graphic_subtitle() == true )
            ret =   overlap_subtitle_image();
        else
            ret =   render_nongraphic_subtitle();
    }    

    // loop control
    if( recv_count - 1 < recv_size )
        return  HAVE_FRAME;
    else
    {
        // 找不到位置塞, 暫時先放這裡.
        av_packet_unref(pkt_bsf);
        return  0;  // break loop.
    }
}




