#include "video_encode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tool.h"
#include <QImage>


extern "C" {

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

}





/*******************************************************************************
VideoEncode::VideoEncode()
********************************************************************************/
VideoEncode::VideoEncode()
    :   Encode()
{}





/*******************************************************************************
VideoEncode::~VideoEncode()
********************************************************************************/
VideoEncode::~VideoEncode()
{}






/*******************************************************************************
VideoEncode::init()

https://www.itread01.com/content/1549629205.html
********************************************************************************/
void    VideoEncode::init( int st_idx, VideoEncodeSetting setting, bool need_global_header )
{
    src_width   =   setting.src_width;
    src_height  =   setting.src_height;

    //
    Encode::init( st_idx, setting.code_id );
    Encode::open();

    int     ret     =   0;

    //
    ctx->bit_rate   =   3000000;
    ctx->width      =   setting.width;
    ctx->height     =   setting.height;

    ctx->time_base  =   setting.time_base; 
    ctx->framerate.num  =   setting.time_base.den; 
    ctx->framerate.den  =   setting.time_base.num;

    ctx->gop_size       =   setting.gop_size;
    ctx->max_b_frames   =   setting.max_b_frames;

    ctx->pix_fmt        =   setting.pix_fmt;
    
    // 底下參數未開放外部設置,之後思考要不要開放
    ctx->me_subpel_quality  =   10;
    if( codec->id == AV_CODEC_ID_H264 || codec->id == AV_CODEC_ID_H265 )
        av_opt_set( ctx->priv_data, "preset", "medium", 0);

    if( ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO )
        ctx->mb_decision    =   FF_MB_DECISION_RD;

    // need before avcodec_open2
    if( need_global_header == true )
        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // open codec.
    //AVDictionary *opt_arg = nullptr;
    //AVDictionary *opt = NULL;
    //av_dict_copy(&opt, opt_arg, 0);
    //ret     =   avcodec_open2( ctx, codec, &opt );

    ret     =   avcodec_open2( ctx, codec, nullptr );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "open fail" );

    // frame setting
    frame->format   =   ctx->pix_fmt;
    frame->width    =   ctx->width;
    frame->height   =   ctx->height;

    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "get buffer fail." );

    //
    init_sws( setting );
}





/*******************************************************************************
VideoEncode::init_sws()
********************************************************************************/
void    VideoEncode::init_sws( VideoEncodeSetting setting )
{
    // data for sws.
    video_bufsize   =   av_image_alloc( video_data, video_linesize, ctx->width, ctx->height, ctx->pix_fmt, 1 );

    sws_ctx     =   sws_getContext( setting.src_width, setting.src_height, setting.src_pix_fmt,    // src
                                    ctx->width,        ctx->height,        ctx->pix_fmt,           // dst
                                    SWS_BICUBIC, NULL, NULL, NULL );
}







/*******************************************************************************
VideoEncode::encode_test()

測試用 目前不能動
********************************************************************************/
void    VideoEncode::encode_test()
{
    if( frame != nullptr )
        printf( "pict type = %c\n", av_get_picture_type_char(frame->pict_type) );

    int ret;

    /* send the frame to the encoder */
    ret = avcodec_send_frame( ctx, frame );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "send fail." );

    while( ret >= 0 ) 
    {
        ret = avcodec_receive_packet( ctx, pkt );
        if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
            return;
        else if( ret < 0 ) 
            MYLOG( LOG::ERROR, "recv fail." );

        printf( "write packet %lld %d\n", pkt->pts, pkt->size );
        //fwrite( pkt->data, 1, pkt->size, output );
        av_packet_unref(pkt);
    }
}




/*******************************************************************************
VideoEncode::send_frame()
********************************************************************************/
int     VideoEncode::send_frame()
{
    //MYLOG( LOG::DEBUG, "pict type = %c\n", av_get_picture_type_char(frame->pict_type) );
    int ret =  Encode::send_frame();
    return  ret;
}









/*******************************************************************************
VideoEncode::work_test()

測試用,目前不能動.
********************************************************************************/
void    VideoEncode::work_test()
{
    //output = fopen( "H:\\test.mp4", "wb+" );

    char    str[1000];
    int     i, ret;

    uint8_t  *video_dst_data[4]     =   { nullptr };
    int      video_dst_linesize[4]  =   { 0 };
    int      video_dst_bufsize      =   0;

    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, 1920, 1080, AV_PIX_FMT_YUV420P, 1 );

    SwsContext* sws_ctx     =   sws_getContext( 1920, 1080, AV_PIX_FMT_BGRA,                     // src
                                                1920, 1080, AV_PIX_FMT_YUV420P,                  // dst
                                                SWS_BICUBIC, NULL, NULL, NULL );    

    //int   y, x;

    //
    for( i = 0; i <= 35719; i++ )
    {
        sprintf( str, "H:\\jpg\\%d.jpg", i );
        printf( "str = %s\n", str );

        QImage img( str );

        ret = av_frame_make_writable(frame);
        if( ret < 0 )
            assert(0);

#if 1
        int linesize[8] = { img.bytesPerLine() };
        uint8_t* ptr[4] = { img.bits() };

        sws_scale( sws_ctx, ptr, linesize, 0, 1080, video_dst_data, video_dst_linesize );

        memcpy( frame->data[0], video_dst_data[0], 1920*1080 );
        memcpy( frame->data[1], video_dst_data[1], 1920*1080/4 );
        memcpy( frame->data[2], video_dst_data[2], 1920*1080/4 );
#else
        for (y = 0; y < ctx->height; y++) {
            for (x = 0; x < ctx->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }

        /* Cb and Cr */
        for (y = 0; y < ctx->height/2; y++) {
            for (x = 0; x < ctx->width/2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }
#endif

        frame->pts  =   i;
        encode_test();
    }

    // flush
    //encode_test( NULL );

    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    //if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
      //  fwrite( endcode, 1, sizeof(endcode), output );

}





/*******************************************************************************
VideoEncode::end()
********************************************************************************/
void    VideoEncode::end()
{
    Encode::end();

    if( video_data[0] != nullptr )
    {
        av_free( video_data[0] );
        video_data[0]   =   nullptr;
        video_data[1]   =   nullptr;
        video_data[2]   =   nullptr;
        video_data[3]   =   nullptr;
    }

    video_linesize[0]   =   0;
    video_linesize[1]   =   0;
    video_linesize[2]   =   0;
    video_linesize[3]   =   0;
    video_bufsize       =   0;
}






/*******************************************************************************
VideoEncode::get_pts()
********************************************************************************/
int64_t     VideoEncode::get_pts()
{
    return  frame_count;
}






/*******************************************************************************
VideoEncode::get_frame()

這邊需要擴充, 以及考慮使用讀取檔案處理一些參數的 init
********************************************************************************/
AVFrame*    VideoEncode::get_frame()
{
    char str[1000];
    int ret;

    sprintf( str, "J:\\jpg\\%d.jpg", frame_count );
    printf( "str = %s\n", str );

    if( frame_count > 300 )
        return  nullptr;

    QImage img;
    if( img.load( str ) == false )
        return  nullptr;    

    ret = av_frame_make_writable( frame );
    if( ret < 0 )
        assert(0);

    int         linesize[8]     =   { img.bytesPerLine() };
    uint8_t     *data[4]         =   { img.bits() };
    
    sws_scale( sws_ctx, data, linesize, 0, img.height(), video_data, video_linesize );

#if 1
    av_image_copy( frame->data, frame->linesize, 
                   (const uint8_t**)video_data, video_linesize, 
                   ctx->pix_fmt, ctx->width, ctx->height );
#else
    // yuv420p
    //memcpy( frame->data[0], video_dst_data[0], ctx->width * ctx->height );
    //memcpy( frame->data[1], video_dst_data[1], ctx->width * ctx->height / 4 );
    //memcpy( frame->data[2], video_dst_data[2], ctx->width * ctx->height / 4 );
    // yuv420p10le
    memcpy( frame->data[0], video_dst_data[0], video_dst_linesize[0] * ctx->height );
    memcpy( frame->data[1], video_dst_data[1], video_dst_linesize[1] * ctx->height / 2);
    memcpy( frame->data[2], video_dst_data[2], video_dst_linesize[2] * ctx->height / 2);
#endif

    frame->pts = frame_count;
    frame_count++;

    return frame;
}
