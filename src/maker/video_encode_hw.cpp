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

參數參考
https://www.jianshu.com/p/b46a33dd958d
https://blog.csdn.net/leiflyy/article/details/87935084
https://blog.csdn.net/NB_vol_1/article/details/78362701

https://www.twblogs.net/a/5c763253bd9eee339917e63a  參數參考 比較喜歡這篇

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

    // Encode::init 使用 avcodec_find_encoder, 無法取得 hw encode. 強制取得nvenc, 重設 codec.
    if( setting.code_id == AV_CODEC_ID_H264 )
        codec   =   avcodec_find_encoder_by_name("h264_nvenc");
    else if( setting.code_id == AV_CODEC_ID_H265 )
        codec   =   avcodec_find_encoder_by_name("hevc_nvenc");
    else
        assert(0);

    // Encode::init 裡面會 create 一份 cpu 的 ctx. 這邊重新 alloc. 有空重構這塊
    if( ctx != nullptr )
    {
        avcodec_free_context( &ctx );
        ctx     =   nullptr;
    }
    ctx     =   avcodec_alloc_context3(codec);

    hw_encoder_init(hw_type);

    // 同時設 cq 跟 bitrate 會互相干擾, 不要同時設
#ifdef FFMPEG_TEST
    //ctx->bit_rate   =   3000000;
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
        //av_opt_set( ctx->priv_data, "preset", "ultrafast",   0 );
        //av_opt_set( ctx->priv_data, "tune",   "zerolatency", 0 );
#endif
    }

    // 類似CRF的參數
    //av_opt_set( ctx, "cq", "20", AV_OPT_SEARCH_CHILDREN );  // 固定品質的設定


    src_width   =   setting.src_width;
    src_height  =   setting.src_height;

    //ctx->me_subpel_quality  =   10;

    ctx->width      =   setting.width;
    ctx->height     =   setting.height;

    if( setting.pix_fmt == AV_PIX_FMT_YUV420P10LE )
        ctx->pix_fmt    =   AV_PIX_FMT_P010LE;  // nvenc 必須設置為 P010
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


