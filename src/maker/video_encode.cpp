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
********************************************************************************/
void    VideoEncode::init( int st_idx, VideoEncodeSetting setting )
{
    int ret;

    Encode::init( st_idx, setting.code_id );

    //
    ctx->bit_rate   =   3000000;
    ctx->width      =   setting.width;
    ctx->height     =   setting.height;

    ctx->time_base.num  =   1001; 
    ctx->time_base.den  =   24000;
    ctx->framerate.num  =   24000; 
    ctx->framerate.den  =   1001;

    ctx->gop_size       =   200;
    ctx->max_b_frames   =   150;
    ctx->pix_fmt        =   AV_PIX_FMT_YUV420P;
    //ctx->me_subpel_quality = 10;

    if( codec->id == AV_CODEC_ID_H264 )
        av_opt_set( ctx->priv_data, "preset", "medium", 0);

#if 0
    // 未來研究這段code的作用
    if( ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) 
    {
        /* just for testing, we also add B-frames */
        ctx->max_b_frames = 2;
    }
    if( ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO )
    {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
        * This does not happen with normal video, it just happens here as
        * the motion of the chroma plane does not match the luma plane. */
        ctx->mb_decision = 2;
    }
#endif

    // open codec.
    ret     =   avcodec_open2( ctx, codec, NULL );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "open fail" );

    // frame setting
    frame->format   =   ctx->pix_fmt;
    frame->width    =   ctx->width;
    frame->height   =   ctx->height;

    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "get buffer fail." );

    // data for sws.
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, ctx->width, ctx->height, ctx->pix_fmt , 1 );

    sws_ctx     =   sws_getContext( 1920, 1080, AV_PIX_FMT_BGRA,                     // src
                                    ctx->width, ctx->height, ctx->pix_fmt,           // dst
                                    SWS_BICUBIC, NULL, NULL, NULL );

    ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
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
    int     ret     =   0;

    //MYLOG( LOG::DEBUG, "pict type = %c\n", av_get_picture_type_char(frame->pict_type) );

    if( frame == nullptr )
    {
        MYLOG( LOG::ERROR, "frame is null." );
        ret =   ERROR;
    }
    else
        ret     =    avcodec_send_frame( ctx, frame );

    return  ret;
}





/*******************************************************************************
VideoEncode::recv_frame()
********************************************************************************/
int     VideoEncode::recv_frame()
{
    if( ctx == nullptr || pkt == nullptr )
        MYLOG( LOG::ERROR, "ctx or pkt == nullptr." );

    int     ret =   avcodec_receive_packet( ctx, pkt );
    return  ret;
}





/*******************************************************************************
VideoEncode::get_pkt()
********************************************************************************/
AVPacket*   VideoEncode::get_pkt()
{
    pkt->stream_index = stream_index;
    return pkt;
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

    if( video_dst_data[0] != nullptr )
    {
        av_free( video_dst_data[0] );
        video_dst_data[0]   =   nullptr;
        video_dst_data[1]   =   nullptr;
        video_dst_data[2]   =   nullptr;
        video_dst_data[3]   =   nullptr;
    }

    video_dst_linesize[0]   =   0;
    video_dst_linesize[1]   =   0;
    video_dst_linesize[2]   =   0;
    video_dst_linesize[3]   =   0;
    video_dst_bufsize       =   0;
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

    if( frame_count > 35719 )
        return nullptr;

    sprintf( str, "J:\\jpg\\%d.jpg", frame_count );
    printf( "str = %s\n", str );

    QImage img( str );    

    ret = av_frame_make_writable(frame);
    if( ret < 0 )
        assert(0);

    int linesize[8] = { img.bytesPerLine() };
    uint8_t* ptr[4] = { img.bits() };
    
    sws_scale( sws_ctx, ptr, linesize, 0, 1080, video_dst_data, video_dst_linesize );
    
    memcpy( frame->data[0], video_dst_data[0], 1920*1080 );
    memcpy( frame->data[1], video_dst_data[1], 1920*1080/4 );
    memcpy( frame->data[2], video_dst_data[2], 1920*1080/4 );

    frame->pts = frame_count;

    frame_count++;

    return frame;
}
