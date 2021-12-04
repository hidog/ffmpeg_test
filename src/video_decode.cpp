#include "video_decode.h"

#include "tool.h"

#include <QImage>
#include <opencv2/opencv.hpp>


extern "C" {

#include <libswscale/swscale.h>

#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

} // end extern "C"






/*******************************************************************************
VideoDecode::VideoDecode()
********************************************************************************/
VideoDecode::VideoDecode()
    :   Decode()
{
    type        =   AVMEDIA_TYPE_VIDEO;
    pix_fmt     =   AV_PIX_FMT_NONE;
    //v_codec_id  =   AV_CODEC_ID_NONE;
}





/*******************************************************************************
VideoDecode::VideoDecode()
********************************************************************************/
AVPixelFormat   VideoDecode::get_pix_fmt()
{
    return  pix_fmt;
}




/*******************************************************************************
VideoDecode::~VideoDecode()
********************************************************************************/
VideoDecode::~VideoDecode()
{
    end();
}







/*******************************************************************************
VideoDecode::open_codec_context()
********************************************************************************/
int     VideoDecode::open_codec_context( AVFormatContext *fmt_ctx )
{
    Decode::open_all_codec( fmt_ctx, type );
    //dec_ctx->thread_count = 4;
    return  SUCCESS;
}






/*******************************************************************************
VideoDecode::init()
********************************************************************************/
int     VideoDecode::init()
{
    int ret     =   0;
    width       =   dec_ctx->width;
    height      =   dec_ctx->height;
    pix_fmt     =   dec_ctx->pix_fmt;

    MYLOG( LOG::INFO, "width = %d, height = %d, pix_fmt = %d\n", width, height, pix_fmt );
    
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_RGB24, 1 );
    if( video_dst_bufsize < 0 )
    {
        MYLOG( LOG::ERROR, "Could not allocate raw video buffer" );
        return  ERROR;
    }

    // NOTE : 可以改變寬高. 
    sws_ctx     =   sws_getContext( width, height, pix_fmt,                     // src
                                    width, height, AV_PIX_FMT_RGB24,            // dst
                                    SWS_BICUBIC, NULL, NULL, NULL);                                    

    //
#ifdef FFMPEG_TEST
    output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_QT, this );
    //output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_openCV, this );
#endif

    //
    Decode::init();

    return  SUCCESS;
}








/*******************************************************************************
VideoDecode::output_decode_info()
********************************************************************************/
void    VideoDecode::output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx )
{
    MYLOG( LOG::INFO, "video dec name = %s", dec->name );
    MYLOG( LOG::INFO, "video dec long name = %s", dec->long_name );
    MYLOG( LOG::INFO, "video dec codec id = %s", avcodec_get_name(dec->id) );
    MYLOG( LOG::INFO, "video bitrate = %lld, pix_fmt = %s", dec_ctx->bit_rate, av_get_pix_fmt_name(dec_ctx->pix_fmt) );
}








/*******************************************************************************
VideoDecode::output_video_frame_info()
********************************************************************************/
void    VideoDecode::output_video_frame_info()
{
    const char  *frame_format_str   =   av_get_pix_fmt_name( static_cast<AVPixelFormat>(frame->format) );

    MYLOG( LOG::INFO, "frame width = %d, height = %d", frame->width, frame->height );
    MYLOG( LOG::INFO, "frame format = %s", frame_format_str );

    char        buf[AV_TS_MAX_STRING_SIZE]{0};
    const char* time_str    =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    MYLOG( LOG::INFO, "video_frame = %d, coded_n : %d, time = %s", frame_count, frame->coded_picture_number, time_str );
}





/*******************************************************************************
VideoDecode::end()
********************************************************************************/
int     VideoDecode::end()
{
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


    if( sws_ctx != nullptr )
    {
        sws_freeContext( sws_ctx );
        sws_ctx     =   nullptr;
    }

    pix_fmt =   AV_PIX_FMT_NONE;
    width   =   0;
    height  =   0;

    Decode::end();
    return  SUCCESS;
}







/*******************************************************************************
VideoDecode::output_video_data()

某些影片不能直接複製到QImage的記憶體,需要先複製到ffmpeg create的video_dst_data.
********************************************************************************/
VideoData   VideoDecode::output_video_data()
{
    VideoData   vd;

    QImage  image { width, height, QImage::Format_RGB888 };

    //av_image_fill_linesizes( linesizes, AV_PIX_FMT_RGB24, frame->width );
    sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, video_dst_data, video_dst_linesize );
    memcpy( image.bits(), video_dst_data[0], video_dst_bufsize );

    //
    vd.index        =   frame_count;
    vd.frame        =   image;
    vd.timestamp    =   get_timestamp();

    return  vd;
}







/*******************************************************************************
VideoDecode::get_video_width()
********************************************************************************/
int     VideoDecode::get_video_width()
{
    return  stream->codecpar->width;
}




/*******************************************************************************
VideoDecode::get_video_height()
********************************************************************************/
int     VideoDecode::get_video_height()
{
    return  stream->codecpar->height;
}







/*******************************************************************************
VideoDecode::get_timestamp()
********************************************************************************/
int64_t     VideoDecode::get_timestamp()
{
    if( frame->best_effort_timestamp == AV_NOPTS_VALUE )
        return  0;

    double  dpts    =   av_q2d(stream->time_base) * frame->best_effort_timestamp;
    int64_t ts      =   dpts * 1000;  // ms
    return  ts;
}




 


/*******************************************************************************
VideoDecode::video_info()

NOTE: 假設影片只有一個視訊軌,先不處理多重視訊軌的問題.
********************************************************************************/
int     VideoDecode::video_info()
{
#if 0
    vs_idx  =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0 );

    //
    AVStream    *video_stream   =   fmt_ctx->streams[vs_idx];
    if( video_stream == nullptr )
    {
        MYLOG( LOG::INFO, "this stream has no video stream" );
        return  SUCCESS;
    }

    //if( fmt_ctx->streams[vs_idx]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
    //  printf("Test");

    //
    AVCodecID   v_codec_id    =   fmt_ctx->streams[vs_idx]->codecpar->codec_id;

    width   =   fmt_ctx->streams[vs_idx]->codecpar->width;
    height  =   fmt_ctx->streams[vs_idx]->codecpar->height;
    depth   =   8;
    if( fmt_ctx->streams[vs_idx]->codecpar->format == AV_PIX_FMT_YUV420P10LE )
        depth = 10;
    if( fmt_ctx->streams[vs_idx]->codecpar->format == AV_PIX_FMT_YUV420P12LE )
        depth = 12;

    MYLOG( LOG::INFO, "width = %d, height = %d, depth = %d", width, height, depth );
    MYLOG( LOG::INFO, "code name = %s", avcodec_get_name(v_codec_id) );

    //
    double  fps     =   av_q2d( fmt_ctx->streams[vs_idx]->r_frame_rate );
    MYLOG( LOG::INFO, "fps = %lf", fps );
#endif

#if 0
    // use for NVDEC
    bool flag1  =   !strcmp( fmt_ctx->iformat->long_name, "QuickTime / MOV" )   ||
        !strcmp( fmt_ctx->iformat->long_name, "FLV (Flash Video)" ) ||
        !strcmp( fmt_ctx->iformat->long_name, "Matroska / WebM" );
    bool flag2  =   codec_id == AV_CODEC_ID_H264 || codec_id == AV_CODEC_ID_HEVC;

    use_bsf     =   flag1 && flag2;
#endif

#if 0
    // use for NVDEC
    if( use_bsf == true )
    {
        const AVBitStreamFilter*  bsf   =   nullptr;
        if( codec_id == AV_CODEC_ID_H264 )
            bsf     =   av_bsf_get_by_name("h264_mp4toannexb");
        else if( codec_id == AV_CODEC_ID_HEVC )
            bsf     =   av_bsf_get_by_name("hevc_mp4toannexb");
        else 
            assert(0);

        av_bsf_alloc( bsf, &v_bsf_ctx );
        v_bsf_ctx->par_in   =   fmt_ctx->streams[vs_idx]->codecpar;
        av_bsf_init( v_bsf_ctx );
    }

#endif




#if 0
    用 bsf 處理 pkt, 再丟給 nv decode 的參考.
    if( use_bsf && pkt->stream_index == vs_idx )
    {
        av_bsf_send_packet( v_bsf_ctx, pkt );
        av_bsf_receive_packet( v_bsf_ctx, pkt_bsf );
    }
    else if( pkt->stream_index == as_idx )
    {
        av_bsf_send_packet( a_bsf_ctx, pkt );
        av_bsf_receive_packet( a_bsf_ctx, pkt );
    }
#endif


    return SUCCESS;
}






/*******************************************************************************
VideoDecode::recv_frame()
********************************************************************************/
int     VideoDecode::recv_frame( int index )
{
    int     ret     =   Decode::recv_frame(index);
    if( ret >= 0 )
        frame->pts  =   frame->best_effort_timestamp; 

    return  ret;
/*
    某個測試影片, 少了這個處理, 造成無法顯示字幕
*/
}



#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecode::output_jpg_by_QT()
********************************************************************************/
int    VideoDecode::output_jpg_by_QT()
{
#if 0
    // 網路範例 某些情況下這段code會失效
    // 1. Get frame and QImage to show 
    QImage  img     =   QImage( width, height, QImage::Format_RGB888 );

    // 2. Convert and write into image buffer  
    uint8_t *dst[]  =   { img.bits() };
    int     linesizes[4];

    av_image_fill_linesizes( linesizes, AV_PIX_FMT_RGB24, frame->width );
    sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, dst, linesizes );

    static int  jpg_count   =   0;
    char str[1000];
    sprintf( str, "I:\\%d.jpg", jpg_count++ );
    img.save(str);

    return  0;
#else
    QImage  img     =   QImage( width, height, QImage::Format_RGB888 );
    
    sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, video_dst_data, video_dst_linesize );
    memcpy( img.bits(), video_dst_data[0], video_dst_bufsize );

    static int  jpg_count   =   0;
    char str[1000];
    sprintf( str, "H:\\%d.jpg", jpg_count++ );
    img.save(str);

    return  0;
#endif
}
#endif







#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecode::output_jpg_by_openCV()
********************************************************************************/
int     VideoDecode::output_jpg_by_openCV()
{
    // note: frame 本身有帶 width, height 的資訊
    //int width = frame->width, height = frame->height;

    /* 
    yuv420 本身的資料是 width * height * 3 / 2, 一開始沒處理好造成錯誤
    10bit, 12bit應該會出問題,到時候再研究.
    有兩個方法可以做轉換. 一個比較暴力, 一個是透過介面做轉換
    */
#if 0
    // 某些情況下這段程式碼會出錯.
    // 那時候 linesize 會出現不match的現象
    cv::Mat     img     =   cv::Mat::zeros( height*3/2, width, CV_8UC1 );    
    memcpy( img.data, frame->data[0], width*height );
    memcpy( img.data + width*height, frame->data[1], width*height/4 );
    memcpy( img.data + width*height*5/4, frame->data[2], width*height/4 );
#else
    av_image_copy( video_dst_data, video_dst_linesize, (const uint8_t **)(frame->data), frame->linesize, static_cast<AVPixelFormat>(pix_fmt), width, height );
    cv::Mat img( cv::Size( width, height*3/2 ), CV_8UC1, video_dst_data[0] );
#endif

    cv::Mat     bgr;
    cv::cvtColor( img, bgr, cv::COLOR_YUV2BGR_I420 );
    cv::imshow( "RGB frame", bgr );
    cv::waitKey(1);

    return 0;
}
#endif
