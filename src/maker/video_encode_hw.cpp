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
VideoEncodeHW::end()
********************************************************************************/
void    VideoEncodeHW::end()
{
    VideoEncode::end();

    hw_pix_fmt  =   AV_PIX_FMT_NONE;
    hw_type     =   AV_HWDEVICE_TYPE_NONE;

    if( hw_device_ctx != nullptr )
    {
        av_buffer_unref(&hw_device_ctx);
        hw_device_ctx   =   nullptr;
    }
}





/*******************************************************************************
VideoEncodeHW::init()

把计把σ
https://www.jianshu.com/p/b46a33dd958d
https://blog.csdn.net/leiflyy/article/details/87935084
https://blog.csdn.net/NB_vol_1/article/details/78362701

https://www.twblogs.net/a/5c763253bd9eee339917e63a  把计把σ ゑ耕尺舧硂絞

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

    // ┏祘Α絏 copy from VideoEncode::init э. Τ癘眔篶
#ifdef FFMPEG_TEST
    load_jpg_root_path  =   setting.load_jpg_root_path;
#endif

    Encode::init( st_idx, setting.code_id );

    // Encode::init ㄏノ avcodec_find_encoder, 礚猭眔 hw encode. 眏眔nvenc, 砞 codec.
    if( setting.code_id == AV_CODEC_ID_H264 )
        codec   =   avcodec_find_encoder_by_name("h264_nvenc");
    else if( setting.code_id == AV_CODEC_ID_H265 )
        codec   =   avcodec_find_encoder_by_name("hevc_nvenc");
    else
        assert(0);

    // Encode::init 柑穦 create  cpu  ctx. 硂娩穝 alloc. Τ篶硂遏
    if( ctx != nullptr )
    {
        avcodec_free_context( &ctx );
        ctx     =   nullptr;
    }
    ctx     =   avcodec_alloc_context3(codec);

    hw_encoder_init(hw_type);

    // 砞 cq 蛤 bitrate 穦が耑, ぃ璶砞
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

    // 摸CRF把计
    // av_opt_set( ctx, "cq", "50", AV_OPT_SEARCH_CHILDREN );


    src_width   =   setting.src_width;
    src_height  =   setting.src_height;

    //ctx->me_subpel_quality  =   10;

    ctx->width      =   setting.width;
    ctx->height     =   setting.height;

    if( setting.pix_fmt == AV_PIX_FMT_YUV420P10LE )
        ctx->pix_fmt    =   AV_PIX_FMT_P010LE;  // nvenc ゲ斗砞竚 P010
    else
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


