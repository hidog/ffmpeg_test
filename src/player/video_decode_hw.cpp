#include "video_decode_hw.h"
#include "sub_decode.h"

extern "C" {

#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>

} // end extern "C"



namespace {
AVPixelFormat  g_hw_pix_fmt   =   AV_PIX_FMT_NONE;  // 造成無法 multi-instance. 有空再修吧...
} // end namespace






/*******************************************************************************
VideoDecodeHW::VideoDecodeHW()
********************************************************************************/
VideoDecodeHW::VideoDecodeHW()
    :   VideoDecode(), hw_type{AV_HWDEVICE_TYPE_NONE}, hw_pix_fmt{AV_PIX_FMT_NONE}
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
int     VideoDecodeHW::open_codec_context(  int stream_index, AVFormatContext *fmt_ctx, AVMediaType type  )
{
    list_hw_decoders();

    int     ret     =   0;
    ret     =   find_hw_device_type();

    // open context
    AVCodec     *dec    =   nullptr;
    ret     =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, stream_index, -1, (AVCodec**)&dec, 0);  
    if( ret < 0 ) 
    {
        MYLOG( LOG::L_ERROR, "Cannot find a video stream in the input file. ret = %d", ret );
        return  R_ERROR;
    }

    create_hw_decoder( dec );    
    stream  =   fmt_ctx->streams[stream_index];

    ret     =   avcodec_parameters_to_context( dec_ctx, stream->codecpar );
    if( ret < 0 )
    {
        MYLOG( LOG::L_ERROR, "avcodec_parameters_to_context fail. ret = %d", ret );
        return  R_ERROR;
    }

    //
    ret     =   hw_decoder_init( hw_type );
    if( ret != R_SUCCESS )
    {
        MYLOG( LOG::L_ERROR, "hw_decoder_init fail." );
        return  ret;
    }

    //
    ret     =   avcodec_open2( dec_ctx, dec, NULL );
    if( ret < 0 ) 
    {
        MYLOG( LOG::L_ERROR, "Failed to open codec for stream. ret = %d", ret );
        return  R_ERROR;
    }

    // note: 寫法跟 video_decode_nw 有點不同, 當測試.
    if( dec_ctx->pix_fmt == AV_PIX_FMT_YUV420P10LE )
        dec_ctx->pix_fmt    =   AV_PIX_FMT_P016LE;  // 強制指定fmt
    else if( dec_ctx->pix_fmt == AV_PIX_FMT_YUV420P )
        dec_ctx->pix_fmt    =   AV_PIX_FMT_NV12;
    else
    {
        MYLOG( LOG::L_ERROR, "un handle pix fmt." );
    }

    return  ret;
}






/*******************************************************************************
get_hw_format
********************************************************************************/
AVPixelFormat   get_hw_format( AVCodecContext* ctx, const AVPixelFormat* pix_fmts )
{
    assert(0); // nvdec 似乎不會進到這個 function, 檢查一下

    AVPixelFormat   format  =   AV_PIX_FMT_NONE;
    const AVPixelFormat     *ptr    =   nullptr;

    for( ptr = pix_fmts; *ptr != AV_PIX_FMT_NONE; ptr++ ) 
    {
        if( *ptr == g_hw_pix_fmt )
        {
            format  =   *ptr;
            break;
        }
    }

    if( AV_PIX_FMT_NONE == format )
        MYLOG( LOG::L_ERROR, "Failed to get HW surface format." );

    return  format;
}





/*******************************************************************************
VideoDecodeHW::create_hw_decoder
********************************************************************************/
int     VideoDecodeHW::create_hw_decoder( AVCodec* dec )
{
    if( dec == nullptr )
        MYLOG( LOG::L_ERROR, "dec is nullptr" )

    //
    const AVCodecHWConfig     *config     =   nullptr;
    int     flag;

    for( int i = 0; ; i++ )
    {
        config  =   avcodec_get_hw_config( dec, i );

        if( nullptr == config ) 
        {
            MYLOG( LOG::L_ERROR, "Decoder %s does not support device type %s.\n", dec->name, av_hwdevice_get_type_name(hw_type) );
            return  R_ERROR;
        }

        flag    =   config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX;
        if( flag != 0 && config->device_type == hw_type )
        {
            hw_pix_fmt      =   config->pix_fmt;
            g_hw_pix_fmt    =   hw_pix_fmt;
            break;
        }
    }

    MYLOG( LOG::L_INFO, "hw_dev = %s, hw_pix_fmt = %s", hw_dev.c_str(), av_get_pix_fmt_name(hw_pix_fmt) );

    //
    dec_ctx     =   avcodec_alloc_context3(dec);
    if( dec_ctx == nullptr )
    {
        MYLOG( LOG::L_ERROR, "dec_ctx is nullptr. err = %d",  AVERROR(ENOMEM) );
        return  R_ERROR;
    }

    return  R_SUCCESS;
}






/*******************************************************************************
VideoDecodeHW::find_hw_device_type
********************************************************************************/
int     VideoDecodeHW::find_hw_device_type()
{
    hw_type     =   av_hwdevice_find_type_by_name( hw_dev.c_str() );

    if( hw_type == AV_HWDEVICE_TYPE_NONE ) 
    {
        MYLOG( LOG::L_ERROR, "fine hw type %s fail.", hw_dev.c_str() );
        return  R_ERROR;
    }

    return  R_SUCCESS;
}





/*******************************************************************************
VideoDecodeHW::hw_decoder_init
********************************************************************************/
int     VideoDecodeHW::hw_decoder_init( const AVHWDeviceType type )
{
    int     ret     =   0;

    ret     =   av_hwdevice_ctx_create( &hw_device_ctx, type, NULL, NULL, 0 );

    if( ret < 0 ) 
    {
        MYLOG( LOG::L_ERROR, "Failed to create specified HW device. ret = %d", ret );
        return  R_ERROR;
    }

    dec_ctx->hw_device_ctx  =   av_buffer_ref(hw_device_ctx);

    return  R_SUCCESS;
}




/*******************************************************************************
VideoDecodeHW::init

寫得有點醜 有空整理這邊的程式碼
********************************************************************************/
int     VideoDecodeHW::init()
{
    list_hw_decoders();
    
    int     ret =   0;
    
    // 底下程式碼從 VideoDecode::init() 複製過來修改. 有空整理
    width       =   dec_ctx->width;
    height      =   dec_ctx->height;
    pix_fmt     =   dec_ctx->pix_fmt;

    MYLOG( LOG::L_INFO, "width = %d, height = %d, pix_fmt = %d\n", width, height, pix_fmt );
    
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_RGB24, 1 );
    //video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_YUV420P, 1 );  // when use output_jpg_by_openCV, use this.

    if( video_dst_bufsize < 0 )
    {
        MYLOG( LOG::L_ERROR, "Could not allocate raw video buffer" );
        return  R_ERROR;
    }

    // NOTE : 可以改變寬高. 
    AVPixelFormat   frame_fmt   =   AV_PIX_FMT_NONE;    
    if( pix_fmt == AV_PIX_FMT_NV12 )
        frame_fmt  =   AV_PIX_FMT_YUV420P; 
    else if( pix_fmt == AV_PIX_FMT_P016LE )
        frame_fmt  =   AV_PIX_FMT_YUV420P16LE;
    else
        assert(0);
    sws_ctx     =   sws_getContext( width, height, frame_fmt,                   // src
                                    width, height, AV_PIX_FMT_RGB24,            // dst
                                    SWS_BICUBIC, NULL, NULL, NULL);                        

    if( sws_ctx == nullptr )
    {
        MYLOG( LOG::L_ERROR, "sws init fail." );
    }

#ifdef FFMPEG_TEST
    output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_QT, this );
    //output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_openCV, this );
    //output_frame_func   =   std::bind( &VideoDecode::test_image_process, this );
#endif

    Decode::init();

    // 理論上可以在這之前就設置好 sub_dec, 但目前規劃是 init 後再設置 sub_dec.
    if( sub_dec != nullptr )
        MYLOG( LOG::L_ERROR, "sub_dec not null." );

    // 到這邊為止, copy from VideoDecode::init  應該可以包裝得更好一點, 有空修

    // member
    assert( hw_frame == nullptr );
    hw_frame    =   av_frame_alloc();
    if( hw_frame == nullptr ) 
    {
        MYLOG( LOG::L_ERROR, "Could not allocate hw_frame" );
        ret =   AVERROR(ENOMEM);        
        return  ret;
    }

    assert( sw_frame == nullptr );
    sw_frame    =   av_frame_alloc();
    if( sw_frame == nullptr ) 
    {
        MYLOG( LOG::L_ERROR, "Could not allocate sw_frame" );
        ret =   AVERROR(ENOMEM);        
        return  ret;
    }

    // init sws
    // 逆推 fmt. 有空包成函數
    hw_sws_ctx     =   sws_getContext( width, height, pix_fmt,                     // src
                                       width, height, frame_fmt,                   // dst
                                       SWS_BICUBIC, NULL, NULL, NULL);

    return  R_SUCCESS;
}




/*******************************************************************************
VideoDecodeHW::end
********************************************************************************/
int     VideoDecodeHW::end()
{
    VideoDecode::end();

    hw_type     =   AV_HWDEVICE_TYPE_NONE;
    hw_pix_fmt  =   AV_PIX_FMT_NONE;

    if( hw_device_ctx != nullptr )
    {
        av_buffer_unref(&hw_device_ctx);
        hw_device_ctx   =   nullptr;
    }

    if( hw_frame != nullptr )
    {
        av_frame_free(&hw_frame);
        hw_frame    =   nullptr;
    }

    if( sw_frame != nullptr )
    {
        av_frame_free(&sw_frame);
        sw_frame    =   nullptr;
    }

    if( hw_sws_ctx != nullptr )
    {
        sws_freeContext( hw_sws_ctx );
        hw_sws_ctx  =   nullptr;
    }

    return  R_SUCCESS;
}




/*******************************************************************************
VideoDecodeHW::flush_for_seek
********************************************************************************/
void    VideoDecodeHW::flush_for_seek()
{
    Decode::flush_for_seek();
}




/*******************************************************************************
VideoDecodeHW::recv_frame
********************************************************************************/
int     VideoDecodeHW::recv_frame( int index )
{
    int     ret     =   0;

    //
    ret     =   avcodec_receive_frame( dec_ctx, hw_frame );
    if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
        return  0;  
    else if( ret < 0 ) 
    {
        MYLOG( LOG::L_ERROR, "avcodec_receive_frame fail, ret = %d", ret );
        return  ret;
    }
    
    //
    frame_count++;       // 這個參數錯掉會影響後續播放.

    //
    if( hw_frame->format == hw_pix_fmt ) 
    {
        // retrieve data from GPU to CPU 
        ret    =   av_hwframe_transfer_data( sw_frame, hw_frame, 0 );
        if( ret < 0 ) 
        {
            MYLOG( LOG::L_ERROR, "Error transferring the data to system memory. ret = %d", ret );
            return  R_ERROR;
        }
    } 
    else
    {
        MYLOG( LOG::L_ERROR, "frame->format != hw_pix_fmt" );
    }
    
    //
    frame->format   =   get_pix_fmt();
    frame->width    =   width;
    frame->height   =   height;

    // allocate frame data.
    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "get buffer fail." );
    
    ret     =   av_frame_make_writable(frame);
    if( ret < 0 )
        assert(0);
    
    // 逆推 fmt. 寫得不好. 很多累贅. 有空修
    AVPixelFormat   hw_fmt  =   AV_PIX_FMT_NONE;
    if( frame->format == AV_PIX_FMT_YUV420P )
        hw_fmt  =   AV_PIX_FMT_NV12;
    else if( frame->format == AV_PIX_FMT_YUV420P16LE )
        hw_fmt  =   AV_PIX_FMT_P016LE;
    else
        assert(0);

    sws_scale( hw_sws_ctx, sw_frame->data, (const int*)sw_frame->linesize, 0, sw_frame->height, frame->data, frame->linesize );

    //
    //MYLOG( LOG::L_DEBUG, "ctx timebase = (%d/%d)", dec_ctx->time_base.num, dec_ctx->time_base.den );
    //MYLOG( LOG::L_DEBUG, "stream timebase = (%d/%d)", stream->time_base.num, stream->time_base.den );
    frame->best_effort_timestamp    =   hw_frame->best_effort_timestamp;
    frame->pts                      =   hw_frame->best_effort_timestamp;  // need set frame pts, otherwise, 不然部分檔案會出現字幕閃爍
    frame_pts                       =   hw_frame->best_effort_timestamp;
    
    if( sub_dec != nullptr )
    {
        if( sub_dec->is_graphic_subtitle() == true )
            ret     =   overlap_subtitle_image();
        else
            ret     =   render_nongraphic_subtitle();
    }
    
    return  HAVE_FRAME;
}




/*******************************************************************************
VideoDecodeHW::get_pix_fmt

AV_PIX_FMT_P016LE
AV_PIX_FMT_YUV420P10LE

寫法跟 video decode nv 不同, 實驗看看.
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
        MYLOG( LOG::L_ERROR, "not yuv420p, yuv420p10" );
        return  AV_PIX_FMT_YUV420P16LE;    
    }
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
        ret =   recv_frame(-1);  // flush 階段必須傳入 < 0 的值
        if( ret <= 0 ) 
            break;
    
        // write the frame data to output file
        output_frame_func();     
        av_frame_unref(frame);
    }
    
    return 0;
}
#endif


