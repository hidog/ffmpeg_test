#include "video_encode_hw.h"
#include "tool.h"

#include <cassert>

extern "C" {

#include <libavutil/hwcontext.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

} // end extern "C"





/*******************************************************************************
VideoEncodeHW::VideoEncodeHW()
********************************************************************************/
VideoEncodeHW::VideoEncodeHW()
    :   VideoEncode(), hw_type(AV_HWDEVICE_TYPE_NONE), hw_pix_fmt(AV_PIX_FMT_NONE)
{}






/*******************************************************************************
VideoEncodeHW::~VideoEncodeHW()
********************************************************************************/
VideoEncodeHW::~VideoEncodeHW()
{}






/*******************************************************************************
VideoEncodeHW::create_hw_encoder
********************************************************************************/
int     VideoEncodeHW::create_hw_encoder( AVCodec* enc )
{
    if( enc == nullptr )
        MYLOG( LOG::L_ERROR, "enc is nullptr" )

    hw_pix_fmt  =   AV_PIX_FMT_CUDA;
    MYLOG( LOG::L_INFO, "hw_dev = %s, hw_pix_fmt = %s", hw_dev.c_str(), av_get_pix_fmt_name(hw_pix_fmt) );

    //
    ctx     =   avcodec_alloc_context3(enc);
    if( ctx == nullptr )
    {
        MYLOG( LOG::L_ERROR, "ctx is nullptr. err = %d",  AVERROR(ENOMEM) );
        return  R_ERROR;
    }

    return  R_SUCCESS;
}




/*******************************************************************************
VideoEncodeHW::hw_encoder_init
********************************************************************************/
int     VideoEncodeHW::hw_encoder_init( const AVHWDeviceType type )
{
    int     ret     =   0;

    ret     =   av_hwdevice_ctx_create( &hw_device_ctx, type, NULL, NULL, 0 );

    if( ret < 0 ) 
    {
        MYLOG( LOG::L_ERROR, "Failed to create specified HW device. ret = %d", ret );
        return  R_ERROR;
    }

    ctx->hw_device_ctx  =   av_buffer_ref(hw_device_ctx);
    assert( ctx->hw_device_ctx != nullptr );

    return  R_SUCCESS;
}




/*******************************************************************************
VideoEncodeHW::init()
********************************************************************************/
void    VideoEncodeHW::init( int st_idx, VideoEncodeSetting setting, bool need_global_header )
{
    list_hw_decoders();

    //
    int     ret     =   0;
    ret     =   find_hw_device_type();
    if( ret != R_SUCCESS )    
    {
        MYLOG( LOG::L_ERROR, "find_hw_device_type fail." );
    }

    // 底下程式碼 copy from VideoEncode::init 後修改. 有空記得重構
#ifdef FFMPEG_TEST
    load_jpg_root_path  =   setting.load_jpg_root_path;
#endif

    Encode::init( st_idx, setting.code_id );
    codec   =   avcodec_find_encoder_by_name("h264_nvenc");  // Encode::init 使用 avcodec_find_encoder, 無法取得 hw encode. 強制取得nvenc

    // 
    create_hw_encoder(codec);
    hw_encoder_init(hw_type);

    //
    assert( hw_frame == nullptr );
    av_hwframe_get_buffer( ctx->hw_device_ctx, hw_frame, 0 );

    // 底下參數未開放外部設置,之後思考要不要開放    
#ifdef FFMPEG_TEST
    ctx->bit_rate   =   3000000;
#else
    ctx->bit_rate   =   3000000;
#endif

    /*
        fast, medium, slow               
    */
    if( codec->id == AV_CODEC_ID_H264 || codec->id == AV_CODEC_ID_H265 )
    {
#ifdef FFMPEG_TEST
        av_opt_set( ctx->priv_data, "preset", "slow",    0 );
#else
        av_opt_set( ctx->priv_data, "preset", "ultrafast",   0 );
        av_opt_set( ctx->priv_data, "tune",   "zerolatency", 0 );
#endif
    }

    src_width   =   setting.src_width;
    src_height  =   setting.src_height;

    //ctx->me_subpel_quality  =   10;

    ctx->width      =   setting.width;
    ctx->height     =   setting.height;
    ctx->pix_fmt    =   setting.pix_fmt;

    ctx->time_base  =   setting.time_base; 
    ctx->framerate.num  =   setting.time_base.den; 
    ctx->framerate.den  =   setting.time_base.num;

    ctx->gop_size       =   setting.gop_size;
    ctx->max_b_frames   =   setting.max_b_frames;
    
    if( ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO )
        ctx->mb_decision    =   FF_MB_DECISION_RD;

    // need before avcodec_open2
    if( need_global_header == true )
        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret     =   avcodec_open2( ctx, codec, nullptr );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "open fail" );

#ifdef FFMPEG_TEST
    // frame setting
    frame->format   =   ctx->pix_fmt;
    frame->width    =   ctx->width;
    frame->height   =   ctx->height;

    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "get buffer fail." );

    //
    init_sws( setting );
#endif
}





/*******************************************************************************
VideoEncodeHW::send_frame
********************************************************************************/
int     VideoEncodeHW::send_frame()
{
    //av_hwframe_transfer_data( hw_frame, frame, 0 );
    int ret =   avcodec_send_frame( ctx, frame );
    return  ret;
}




/*******************************************************************************
VideoEncodeHW::list_hw_decoders
********************************************************************************/
void    VideoEncodeHW::list_hw_decoders()
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
VideoEncodeHW::find_hw_device_type
********************************************************************************/
int     VideoEncodeHW::find_hw_device_type()
{
    hw_type     =   av_hwdevice_find_type_by_name( hw_dev.c_str() );

    if( hw_type == AV_HWDEVICE_TYPE_NONE ) 
    {
        MYLOG( LOG::L_ERROR, "fine hw type %s fail.", hw_dev.c_str() );
        return  R_ERROR;
    }

    return  R_SUCCESS;
}


