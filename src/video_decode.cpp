#include "video_decode.h"

#include "tool.h"

#include <QImage>
#include <opencv2/opencv.hpp>


extern "C" {

#include <libswscale/swscale.h>

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
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
{}







/*******************************************************************************
VideoDecode::open_codec_context()
********************************************************************************/
int     VideoDecode::open_codec_context( int stream_index, AVFormatContext *fmt_ctx )
{
    Decode::open_codec_context( stream_index, fmt_ctx, type );
    dec_ctx->thread_count = 10;
    return  SUCCESS;
}






/*******************************************************************************
VideoDecode::init()
********************************************************************************/
int     VideoDecode::init()
{
    int ret     =   0;
    // allocate image where the decoded image will be put
    width       =   dec_ctx->width;
    height      =   dec_ctx->height;
    pix_fmt     =   dec_ctx->pix_fmt;

    MYLOG( LOG::INFO, "width = %d, height = %d, pix_fmt = %d\n", width, height, pix_fmt );
    
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, pix_fmt, 1 );
    if( video_dst_bufsize < 0 )
    {
        MYLOG( LOG::ERROR, "Could not allocate raw video buffer" );
        return  ERROR;
    }

    // 先讓 dst 的寬高等於 src 的寬高
                                    // src                  // dst
    sws_ctx     =   sws_getContext( width, height, pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);                                    

    //
#ifdef FFMPEG_TEST
    //output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_QT, this );
    output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_openCV, this );
#endif

    //
    Decode::init();

    return  SUCCESS;
}



#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecode::output_jpg_by_QT()
********************************************************************************/
int    VideoDecode::output_jpg_by_QT()
{
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
}
#endif






/*******************************************************************************
VideoDecode::output_decode_info()
********************************************************************************/
void    VideoDecode::output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx )
{
    MYLOG( LOG::INFO, "video dec name = %s", dec->name );
    MYLOG( LOG::INFO, "video dec long name = %s", dec->long_name );
    MYLOG( LOG::INFO, "video dec codec id = %s", avcodec_get_name(dec->id) );

    MYLOG( LOG::INFO, "video bitrate = %d, pix_fmt = %s", dec_ctx->bit_rate, av_get_pix_fmt_name(dec_ctx->pix_fmt) );
}






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
    av_free( video_dst_data[0] );
    sws_freeContext( sws_ctx );
    Decode::end();
    return  SUCCESS;
}







/*******************************************************************************
VideoDecode::output_video_data()
********************************************************************************/
VideoData   VideoDecode::output_video_data()
{
    VideoData   vd;

    // 1. Get frame and QImage to show 
    QImage  img { width, height, QImage::Format_RGB888 };

    // 2. Convert and write into image buffer  
    uint8_t *dst[]  =   { img.bits() };
    int     linesizes[4];

    av_image_fill_linesizes( linesizes, AV_PIX_FMT_RGB24, frame->width );
    sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, dst, linesizes );

    //
    vd.index        =   frame_count;
    vd.frame        =   img;
    vd.timestamp    =   get_timestamp();

    return  vd;
}







/*******************************************************************************
VideoDecode::get_timestamp()

試了很多方法都沒成功
最後選擇用土法煉鋼

AVRational avr = {1, AV_TIME_BASE};

需要拿浮動 fps 的影片做測試, 應該會出問題

https://blog.csdn.net/chinabinlang/article/details/49885765
********************************************************************************/
int64_t     VideoDecode::get_timestamp()
{
    int64_t ts;

    // MYLOG( LOG::DEBUG, "frame rate = %lf", dec_ctx->framerate ); 印出的數字不是 fps

    //int     num     =   dec_ctx->time_base.num;
    //int     den     =   dec_ctx->time_base.den;

    double  base_ms =   1000.f;  // 表示單位為 mili sec

    //int     interlaced  =   frame->interlaced_frame;
    //MYLOG( LOG::DEBUG, "is interlaced = %d", interlaced );

    int     tpf     =   dec_ctx->ticks_per_frame;

    /* 
        不知道為何要乘 2, 還沒搜尋到原因
        fps = 23.97  ( 24000 / 1001 ) 的時候預期 den = 24000, 實際上卻是 48000
        某些情況又不用 * 2, 是否跟1080p, 1080i 有關??

        https://stackoverflow.com/questions/12234949/ffmpeg-time-unit-explanation-and-av-seek-frame-method
        http://runxinzhi.com/welhzh-p-4835864.html  似乎可以參考這個做判斷, 跟interlace無關

        https://blog.csdn.net/chinabinlang/article/details/49885765
    */
    ts = base_ms * frame_count * den / num;   // 改抓其他參數但必須顛倒. 參考 r_frame_rate

    //ts = base_ms * frame_count * num / den;   // 改從其他位置抓num, den, 但這塊程式碼應該會需要修改
    //ts = base_ms * frame_count *  tpf * num / den;  


    //MYLOG( LOG::DEBUG, "pts = %d", frame->pts );
    //MYLOG( LOG::DEBUG, "coded number = %d", frame->coded_picture_number );

    return ts;
}




 
