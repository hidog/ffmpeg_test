#include "video_decode_nv.h"

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
VideoDecodeNV::VideoDecodeNV()
********************************************************************************/
VideoDecodeNV::VideoDecodeNV()
    :   VideoDecode()
{}








/*******************************************************************************
VideoDecodeNV::~VideoDecodeNV()
********************************************************************************/
VideoDecodeNV::~VideoDecodeNV()
{
    end();
}




#if 0
/*******************************************************************************
VideoDecodeNV::open_codec_context
********************************************************************************/
int     VideoDecodeNV::open_codec_context( AVFormatContext *fmt_ctx )
{
    int     ret     =   VideoDecode::open_codec_context( fmt_ctx );

    AVCodecID       codec_id    =   stream->codecpar->codec_id;
    std::string     log_name    =   fmt_ctx->iformat->long_name;

    bool    flag1   =   log_name == "QuickTime / MOV"    ||
                        log_name == "FLV (Flash Video)"  ||
                        log_name == "Matroska / WebM" ;

    bool    flag2   =   codec_id == AV_CODEC_ID_H264 || codec_id == AV_CODEC_ID_HEVC;

    use_bsf     =   flag1 && flag2;

    if( use_bsf == false )
        MYLOG( LOG::L_ERROR, "not support nv decode.");  // 測試一下不支援的影片會發生甚麼事情

    // NOTE: hardware decode need init bsf.
    init_bsf(fmt_ctx);

    return  ret;
}
#endif




/*******************************************************************************
VideoDecodeNV::init

ffplay -f rawvideo -pix_fmt nv12 -video_size 1280x720 output.data
ffplay -f rawvideo -pix_fmt p016 -video_size 1920x1080 2output.data

AV_PIX_FMT_P016LE
AV_PIX_FMT_NV12
********************************************************************************/
int     VideoDecodeNV::init()
{
    assert( stream != nullptr );

    pkt_bsf     =   av_packet_alloc();
    width       =   stream->codecpar->width;
    height      =   stream->codecpar->height;

    // init nvidia decoder.
    init_nvidia_decoder();

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
VideoDecodeNV::codec_id_ffmpeg_to_cuda
********************************************************************************/
cudaVideoCodec  VideoDecodeNV::codec_id_ffmpeg_to_cuda(AVCodecID id) 
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
VideoDecodeNV::init_nvidia_decoder
********************************************************************************/
int     VideoDecodeNV::init_nvidia_decoder()
{
    AVCodecID       codec_id    =   stream->codecpar->codec_id;  //  dec_ctx->codec_id;
    cudaVideoCodec  cuda_codec  =   codec_id_ffmpeg_to_cuda(codec_id);

    cuInit(0);
    int     gpu_index   =   0; // 使用第一張顯示卡
    int     gpu_num     =   0;
    cuDeviceGetCount( &gpu_num );
    if( gpu_index >= gpu_num ) 
    {
        MYLOG( LOG::L_ERROR, "no video card." );
        return  R_ERROR;
    }

    CUdevice    nv_dev  =   0;
    cuDeviceGet( &nv_dev, gpu_index );

    char    device_name[80];
    cuDeviceGetName( device_name, sizeof(device_name), nv_dev );
    MYLOG( LOG::L_INFO, "gpu use %s", device_name )

    CUcontext   cuda_ctx    =   nullptr;
    cuCtxCreate( &cuda_ctx, 0, nv_dev );

    //
    Rect    rect        =   {};
    Dim     resize_dim  =   {};
    nv_decoder  =   new NvDecoder( cuda_ctx, width, height, false, cuda_codec, nullptr, false, false, &rect, &resize_dim );

    return  R_SUCCESS;
}



/*******************************************************************************
VideoDecodeNV::get_pix_fmt

AV_PIX_FMT_P016LE
AV_PIX_FMT_YUV420P10LE
********************************************************************************/
AVPixelFormat   VideoDecodeNV::get_pix_fmt()
{
    if( stream->codecpar->format == AV_PIX_FMT_YUV420P )
        return  AV_PIX_FMT_YUV420P;
    else if( stream->codecpar->format == AV_PIX_FMT_YUV420P10LE )
        return  AV_PIX_FMT_YUV420P16LE;
    else
    {
        // note: 理論上能相容 測試看看
        MYLOG( LOG::L_ERROR, "not yuv420p, yuv420p10" );
        return  AV_PIX_FMT_YUV420P16LE;    
    }
}



/*******************************************************************************
VideoDecodeNV::init_bsf
********************************************************************************/
int     VideoDecodeNV::init_bsf( AVFormatContext* fmt_ctx )
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
            MYLOG( LOG::L_ERROR, "un support." );
        }

        ret     =   av_bsf_alloc( bsf, &v_bsf_ctx );
        assert( ret == 0 );

        // 參考 end(), 如果沒改 par_in, 會造成 crash.
        // 參考 av_bsf_alloc, 裡面會 alloc par_in, 所以手動釋放
        avcodec_parameters_free( &v_bsf_ctx->par_in );
        
        
        //v_bsf_ctx->par_in   =   fmt_ctx->streams[cs_index]->codecpar;
        v_bsf_ctx->par_in   =   stream->codecpar; // add decode manager modify this code. need test.
        
        
        
        // avcodec_parameters_copy( v_bsf_ctx->par_in, fmt_ctx->streams[cs_index]->codecpar );  嘗試用 copy, 會 crash.

        av_bsf_init( v_bsf_ctx );
    }

    return  R_SUCCESS;
}






/*******************************************************************************
VideoDecodeNV::end
********************************************************************************/
int     VideoDecodeNV::end()
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
        av_bsf_flush( v_bsf_ctx );
        v_bsf_ctx->par_in   =   nullptr;  // 指向 fmt_ctx, 不設成 nullptr 會造成 fmt_ctx 釋放的時候 crash.
        av_bsf_free( &v_bsf_ctx );
        v_bsf_ctx   =   nullptr;
    }
 
    //
    use_bsf     =   false;  
    recv_size   =   0;
    recv_count  =   0; 
    p_timestamp =   nullptr;

    return  R_SUCCESS;
}





/*******************************************************************************
VideoDecodeNV::send_packet
********************************************************************************/
int     VideoDecodeNV::send_packet( AVPacket *pkt )
{
    // 目前在 recv_frame 做 unref. 如果忘了做, yuv420p10 的影片會出錯

    uint8_t     *pkt_ptr    =   nullptr;
    int         pkt_size    =   0;
    int64_t     pkt_ts      =   0;

    if( use_bsf == true )
    {
        av_bsf_send_packet( v_bsf_ctx, pkt );
        av_bsf_receive_packet( v_bsf_ctx, pkt_bsf );

        if( pkt != nullptr )
            pkt->stream_index   =   pkt_bsf->stream_index;   // 否則 stream_index 會被清掉, 造成bug.

        pkt_ptr     =   pkt_bsf->data;
        pkt_size    =   pkt_bsf->size;
    }
    else
    {
        pkt_ptr     =   pkt->data;
        pkt_size    =   pkt->size;
    }

    // decode
    //recv_size   =   0;
    if( pkt_bsf != nullptr )
        nv_decoder->Decode( pkt_ptr, pkt_size, &nv_frames, &recv_size, CUVID_PKT_TIMESTAMP, &p_timestamp, pkt_bsf->pts );
    else
        nv_decoder->Decode( pkt_ptr, pkt_size, &nv_frames, &recv_size, CUVID_PKT_ENDOFSTREAM, &p_timestamp, pkt_bsf->pts );
    //nv_decoder->Decode( pkt_ptr, pkt_size, &nv_frames, &recv_size );

    // need set count to zero. and use in recv loop.
    recv_count  =   0;
    return  R_SUCCESS;
}




/*******************************************************************************
VideoDecodeNV::flush_for_seek
********************************************************************************/
void    VideoDecodeNV::flush_for_seek()
{
    recv_size   =   0;
    recv_count  =   0;
    av_bsf_flush( v_bsf_ctx );

    // flush nv decode.
    while(true)
    {
        nv_decoder->Decode( nullptr, 0, &nv_frames, &recv_size );
        if( recv_size == 0 )
            break;
    }
}





/*******************************************************************************
VideoDecodeNV::convert_to_planar
********************************************************************************/
void    VideoDecodeNV::convert_to_planar( uint8_t *ptr, int w, int h, int d )
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
VideoDecodeNV::recv_frame
********************************************************************************/
int     VideoDecodeNV::recv_frame( int index )
{
    if( recv_size == 0 || recv_count == recv_size )
    {
        av_packet_unref( pkt_bsf );  // 這邊沒 unref, yuv420p10 會出錯
        
        if( index < 0 )
            return  AVERROR_EOF;  // flush 階段, 沒資料了.
        else
            return  0;  // this loop has no frame.
    }

    if( use_bsf == false )
        MYLOG( LOG::L_ERROR, "use bsf is false." )  // 測試看看 use_bsf == false 的時候程式是否能運行
      
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
        MYLOG( LOG::L_ERROR, "get buffer fail." );
    
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
        av_packet_unref(pkt_bsf);   // 沒 unref, yuv420p10 會出錯
        return  0;  // break loop.
    }
}






#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecodeNV::flush
********************************************************************************/
int     VideoDecodeNV::flush()
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


