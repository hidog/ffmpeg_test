#include "video_encode.h"
#include "tool.h"

#ifdef FFMPEG_TEST
#include <QImage>
#include <opencv2/opencv.hpp>
#endif

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

} // end extern "C"





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
{
    end();
}





#ifdef FFMPEG_TEST
/*******************************************************************************
VideoEncode::set_jpg_root_path()
********************************************************************************/
void    VideoEncode::set_jpg_root_path( std::string path )
{
    load_jpg_root_path  =   path;
}
#endif






/*******************************************************************************
VideoEncode::init()

https://www.itread01.com/content/1549629205.html
https://www.itread01.com/content/1550140412.html  parameter 參考
https://trac.ffmpeg.org/wiki/Encode/H.265
ultrafast, superfast, veryfast, faster, fast, 
medium, slow, slower, veryslow, and placebo
********************************************************************************/
void    VideoEncode::init( int st_idx, VideoEncodeSetting setting, bool need_global_header )
{
#ifdef FFMPEG_TEST
    load_jpg_root_path  =   setting.load_jpg_root_path;
#endif

    Encode::init( st_idx, setting.code_id );

    // 底下參數未開放外部設置,之後思考要不要開放    
#ifdef FFMPEG_TEST
    //ctx->bit_rate   =   3000000;
#else
    //ctx->bit_rate   =   3000000;
#endif

    if( codec->id == AV_CODEC_ID_H264 || codec->id == AV_CODEC_ID_H265 )
    {
#ifdef FFMPEG_TEST
        av_opt_set( ctx->priv_data, "preset", "veryslow",    0 );
#else
        av_opt_set( ctx->priv_data, "preset", "ultrafast",   0 );
        av_opt_set( ctx->priv_data, "tune",   "zerolatency", 0 );
#endif
    }

    av_opt_set_int( ctx, "crf", 20, AV_OPT_SEARCH_CHILDREN );  // 固定品質


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

    // 測試一下是否有效果
    //ctx->thread_count   =   1;

    // open codec.
    //AVDictionary *opt_arg = nullptr;
    //AVDictionary *opt = NULL;
    //av_dict_copy(&opt, opt_arg, 0);
    //ret     =   avcodec_open2( ctx, codec, &opt );   

    int     ret     =   avcodec_open2( ctx, codec, nullptr );
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
VideoEncode::list_pixfmt()
********************************************************************************/
void    VideoEncode::list_pix_fmt( AVCodecID code_id )
{
    const AVCodec*     codec   =   avcodec_find_encoder(code_id);

    const AVPixelFormat  *pix_fmt   =   nullptr;

    pix_fmt    =   codec->pix_fmts;
    if( pix_fmt == nullptr )
    {
        MYLOG( LOG::L_INFO, "can not list.\n" );
        return;
    }

    while( *pix_fmt != AV_PIX_FMT_NONE ) 
    {
        printf( "%s support pix_fmt = %d %s\n", avcodec_get_name(code_id), static_cast<int>(*pix_fmt), av_get_pix_fmt_name(*pix_fmt) );
        pix_fmt++;
    }
    MYLOG( LOG::L_DEBUG, "finish.");
}




/*******************************************************************************
VideoEncode::list_frame_rate()
h264, h265 等沒資料.
********************************************************************************/
void    VideoEncode::list_frame_rate( AVCodecID code_id )
{
    const AVCodec*     codec   =   avcodec_find_encoder(code_id);

    const AVRational  *rt   =   nullptr;

    rt    =   codec->supported_framerates;
    if( rt == nullptr )
    {
        MYLOG( LOG::L_INFO, "can not list.\n" );
        return;
    }

    while( rt->den != 0 || rt->num != 0 ) 
    {
        printf( "%s support frame rate = %d/%d = %lf\n", avcodec_get_name(code_id), rt->num, rt->den, av_q2d(*rt) );
        rt++;
    }
    MYLOG( LOG::L_DEBUG, "finish.");
}




/*******************************************************************************
VideoEncode::unref_data()
********************************************************************************/
void    VideoEncode::unref_data()
{
#ifndef FFMPEG_TEST
    av_frame_free( &frame );
#endif
    Encode::unref_data();
}




#ifdef FFMPEG_TEST
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
#endif





#ifdef FFMPEG_TEST
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
        MYLOG( LOG::L_ERROR, "send fail." );

    while( ret >= 0 ) 
    {
        ret = avcodec_receive_packet( ctx, pkt );
        if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
            return;
        else if( ret < 0 ) 
            MYLOG( LOG::L_ERROR, "recv fail." );

        printf( "write packet %lld %d\n", pkt->pts, pkt->size );
        //fwrite( pkt->data, 1, pkt->size, output );
        av_packet_unref(pkt);
    }
}
#endif









#ifdef FFMPEG_TEST
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

        ret     =   av_frame_make_writable(frame);
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
#endif






/*******************************************************************************
VideoEncode::end()
********************************************************************************/
void    VideoEncode::end()
{
    if( frame_count > 0 && ctx != nullptr )
    {
        MYLOG( LOG::L_INFO, "video encode %d frames.", frame_count );
        int64_t     duration_time   =   1000LL * frame_count * ctx->time_base.num / ctx->time_base.den; // ms
        int64_t     ms              =   duration_time % 1000;
        int64_t     sec             =   duration_time / 1000 % 60;
        int64_t     minute          =   duration_time / 1000 / 60 % 60;
        int64_t     hour            =   duration_time / 1000 / 60 / 60;
        MYLOG( LOG::L_INFO, "video encode duration = [ %2lld : %2lld : %2lld . %3lld", hour, minute, sec, ms );
    }

    Encode::end();

    src_width   =   0;
    src_height  =   0;

#ifdef FFMPEG_TEST
    if( video_data[0] != nullptr )
    {
        av_free( video_data[0] );
        video_data[0]   =   nullptr;  // 看起來不用管這個, 但保險起見都設成 nullptr.
        video_data[1]   =   nullptr;
        video_data[2]   =   nullptr;
        video_data[3]   =   nullptr;
    }

    video_linesize[0]   =   0;
    video_linesize[1]   =   0;
    video_linesize[2]   =   0;
    video_linesize[3]   =   0;
    video_bufsize       =   0;

    if( sws_ctx != nullptr )
    {
        sws_freeContext( sws_ctx );
        sws_ctx     =   nullptr;
    }
#endif
}






/*******************************************************************************
VideoEncode::get_pts()
********************************************************************************/
int64_t     VideoEncode::get_pts()
{
    assert( frame != nullptr );
    return  frame->pts;
}






/*******************************************************************************
VideoEncode::get_frame()

這邊需要擴充, 以及考慮使用讀取檔案處理一些參數的 init
********************************************************************************/
void    VideoEncode::next_frame()
{
#ifdef FFMPEG_TEST
    // get_fram_from_file_QT();
    get_fram_from_file_openCV();
#else
    frame   =   encode::get_video_frame();
    if( frame == nullptr )
        eof_flag    =   true;
#endif
}






#ifdef FFMPEG_TEST
/*******************************************************************************
VideoEncode::get_fram_from_file_QT()
********************************************************************************/
void    VideoEncode::get_fram_from_file_QT()
{
    char str[1000];
    int ret;

    sprintf( str, "%s\\%d.jpg", load_jpg_root_path.c_str(), frame_count );
    printf( "str = %s\n", str );

    QImage  img;
    if( img.load( str ) == false )
    {
        eof_flag    =   true;
        return;
    }

    ret =   av_frame_make_writable( frame );
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
    // 暴力硬幹的測試用code.
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
}
#endif






#ifdef FFMPEG_TEST
/*******************************************************************************
VideoEncode::get_fram_from_file_openCV()

這邊需要擴充, 以及考慮使用讀取檔案處理一些參數的 init
********************************************************************************/
void    VideoEncode::get_fram_from_file_openCV()
{
    char str[1000];
    int ret;

    sprintf( str, "%s\\%d.jpg", load_jpg_root_path.c_str(), frame_count );
    if( frame_count % 100 == 0 )
        MYLOG( LOG::L_DEBUG, "load jpg = %s", str );

    cv::Mat img =   cv::imread( str, cv::IMREAD_COLOR );
    if( img.empty() == true )
    //if( frame_count > 1000 )
    {
        eof_flag    =   true;
        return;
    }

    ret     =   av_frame_make_writable( frame );
    if( ret < 0 )
        assert(0);

    size_t      bytes_per_line  =   img.channels() * img.cols;
    int         linesize[8]     =   { bytes_per_line };
    uint8_t     *data[4]        =   { img.ptr() };

    sws_scale( sws_ctx, data, linesize, 0, img.rows, video_data, video_linesize );
    av_image_copy( frame->data, frame->linesize, (const uint8_t**)video_data, video_linesize, ctx->pix_fmt, ctx->width, ctx->height );

    frame->pts  =   frame_count;
    frame_count++;
}
#endif
