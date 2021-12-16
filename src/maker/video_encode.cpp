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
{}





/*******************************************************************************
VideoEncode::~VideoEncode()
********************************************************************************/
VideoEncode::~VideoEncode()
{}






/*******************************************************************************
VideoEncode::init()
********************************************************************************/
void    VideoEncode::init()
{
    int ret;


    //
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if( codec == nullptr )
        MYLOG( LOG::ERROR, "codec = nullptr." );

    //
    ctx = avcodec_alloc_context3(codec);
    if( ctx == nullptr ) 
        MYLOG( LOG::ERROR, "ctx = nullptr." );

    //
    pkt = av_packet_alloc();
    if( pkt == nullptr )
        MYLOG( LOG::ERROR, "pkt = nullptr." );

    //
    ctx->bit_rate = 8000000;
    ctx->width = 1920;
    ctx->height = 1080;

    ctx->time_base.num = 1001; // = (AVRational){1, 25};
    ctx->time_base.den = 24000;
    ctx->framerate.num = 24000; // = (AVRational){25, 1};
    ctx->framerate.den = 1001;

    /* emit one intra frame every ten frames
    * check frame pict_type before passing frame
    * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
    * then gop_size is ignored and the output of encoder
    * will always be I frame irrespective to gop_size
    */
    ctx->gop_size = 150;
    ctx->max_b_frames = 100;
    ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    if( codec->id == AV_CODEC_ID_H264 )
        av_opt_set( ctx->priv_data, "preset", "slow", 0);

    /* open it */
    ret = avcodec_open2( ctx, codec, NULL );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "open fail" );
    //
    frame = av_frame_alloc();
    if( frame == nullptr ) 
        MYLOG( LOG::ERROR, "frame = nullptr" );

    frame->format = ctx->pix_fmt;
    frame->width  = ctx->width;
    frame->height = ctx->height;

    ret = av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "get buffer fail." );
}




/*******************************************************************************
VideoEncode::encode()
********************************************************************************/
void VideoEncode::encode( AVFrame *frame )
{
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
        fwrite( pkt->data, 1, pkt->size, output );
        av_packet_unref(pkt);
    }
}





/*******************************************************************************
VideoEncode::work()
********************************************************************************/
void VideoEncode::work()
{
    output = fopen( "H:\\test.mp4", "wb+" );

    char str[1000];
    int i, ret;




    uint8_t  *video_dst_data[4]     =   { nullptr };
    int      video_dst_linesize[4]  =   { 0 };
    int      video_dst_bufsize      =   0;

    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, 1920, 1080, AV_PIX_FMT_YUV420P, 1 );

    SwsContext* sws_ctx     =   
        sws_getContext( 1920, 1080, AV_PIX_FMT_BGRA,                     // src
                        1920, 1080, AV_PIX_FMT_YUV420P,            // dst
                        SWS_BICUBIC, NULL, NULL, NULL );    



    

    int y, x;

    /* encode 1 second of video */
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


        frame->pts = i;

        /* encode the image */
        encode( frame );
    }


    /* flush the encoder */
    encode( NULL );


    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite( endcode, 1, sizeof(endcode), output );

}





/*******************************************************************************
VideoEncode::end()
********************************************************************************/
void    VideoEncode::end()
{
    fclose(output);

    avcodec_free_context(&ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);

}


