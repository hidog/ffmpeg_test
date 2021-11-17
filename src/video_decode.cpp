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
{}






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
    //dec_ctx->thread_count = 12;
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
    pix_fmt     =   static_cast<myAVPixelFormat>(dec_ctx->pix_fmt);

    MYLOG( LOG::INFO, "width = %d, height = %d, pix_fmt = %d\n", width, height, pix_fmt );
    
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, static_cast<AVPixelFormat>(pix_fmt), 1 );
    if( video_dst_bufsize < 0 )
    {
        MYLOG( LOG::ERROR, "Could not allocate raw video buffer" );
        return  ERROR;
    }

    // ���� dst ���e������ src ���e��
                                    // src                  // dst
    sws_ctx     =   sws_getContext( width, height, static_cast<AVPixelFormat>(pix_fmt), width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);                                    

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





#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecode::output_jpg_by_openCV()
********************************************************************************/
int     VideoDecode::output_jpg_by_openCV()
{
    // note: frame �������a width, height ����T
    //int width = frame->width, height = frame->height;

    /* 
        yuv420 ��������ƬO width * height * 3 / 2, �@�}�l�S�B�z�n�y�����~
        10bit, 12bit���ӷ|�X���D,��ɭԦA��s.
        ����Ӥ�k�i�H���ഫ. �@�Ӥ���ɤO, �@�ӬO�z�L�������ഫ
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




#if 0
�ݭn��s���Ǹ�T�i�H���ӧQ��
/*******************************************************************************
VideoDecode::output_frame()
********************************************************************************/
int     VideoDecode::output_frame()
{
    if( frame->width != width || 
        frame->height != height ||
        frame->format != static_cast<AVPixelFormat>(pix_fmt) ) 
    {
        // To handle this change, one could call av_image_alloc again and decode the following frames into another rawvideo file.         
        auto str1   =   av_get_pix_fmt_name(static_cast<AVPixelFormat>(pix_fmt));
        auto str2   =   av_get_pix_fmt_name( static_cast<AVPixelFormat>(frame->format) );
        ERRLOG( "Error: Width, height and pixel format have to be "
                "constant in a rawvideo file, but the width, height or "
                "pixel format of the input video changed:\n"
                "old: width = %d, height = %d, format = %s\n"
                "new: width = %d, height = %d, format = %s\n",
                width, height, str1, frame->width, frame->height, str2 );
        return  ERROR;
    }
    
    char    buf[AV_TS_MAX_STRING_SIZE]{0};
    auto str    =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    printf( "video_frame n : %d coded_n : %d, time = %s\n", frame_count++, frame->coded_picture_number, str );

    // copy decoded frame to destination buffer: this is required since rawvideo expects non aligned data     
    av_image_copy( video_dst_data, video_dst_linesize, (const uint8_t **)(frame->data), frame->linesize, static_cast<AVPixelFormat>(pix_fmt), width, height );

    // write to rawvideo file
    fwrite( video_dst_data[0], 1, video_dst_bufsize, dst_fp );

    return 0;
}
#endif




/*******************************************************************************
VideoDecode::end()
********************************************************************************/
int     VideoDecode::end()
{
    av_free( video_dst_data[0] );
    Decode::end();
    return  SUCCESS;
}





#if 0
// �N���Ϊ���T�ಾ�X��
/*******************************************************************************
VideoDecode::print_finish_message()
********************************************************************************/
void    VideoDecode::print_finish_message()
{
    // if( video_stream != nullptr )
    // ���] video stream �@�w�s�b. ���ӦA�ݻݨD�[�J�P�_.
    
    auto str    =   av_get_pix_fmt_name(static_cast<AVPixelFormat>(pix_fmt));
    printf( "Play the output video file with the command:\n"
            "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n", str, width, height, dst_file.c_str() );
}
#endif




/*******************************************************************************
VideoDecode::output_video_data()
********************************************************************************/
VideoData   VideoDecode::output_video_data()
{
    VideoData   vd { 0, nullptr, 0 };

    // 1. Get frame and QImage to show 
    QImage  *img     =   new QImage( width, height, QImage::Format_RGB888 );

    if( img == nullptr )
        MYLOG( LOG::ERROR, "alloc QImage fail." );

    // 2. Convert and write into image buffer  
    uint8_t *dst[]  =   { img->bits() };
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

�դF�ܦh��k���S���\
�̫��ܥΤg�k�ҿ�

********************************************************************************/
int64_t VideoDecode::get_timestamp()
{
    //char    buf[AV_TS_MAX_STRING_SIZE]{0};
    //auto str    =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    //double ts = atof(str);

    int64_t ts;

    int64_t delay = 0;
    AVRational avr = {1, AV_TIME_BASE};

    static int64_t last_pts = AV_NOPTS_VALUE;

    //frame->best_effort_timestamp;

    //int64_t aaaa = frame->best_effort_timestamp;
    //ts = av_rescale_q ( aaaa, formatCtx->streams[videoStream]->time_base, AV_TIME_BASE_Q );
    //ts = av_rescale_q ( aaaa, dec_ctx->time_base, avr );
    //printf("ts = %lld\n", ts );


     //ts = 1000 ms / (dec_ctx->time_base.den / 2 / dec_ctx->time_base.num) 




    if( frame->pts != AV_NOPTS_VALUE )
    {
        //if( last_pts != AV_NOPTS_VALUE )
        {
            /* sleep roughly the right amount of time;
            * usleep is in microseconds, just like AV_TIME_BASE. */

            /*
                �����ճo��  2 * ���z��, ���o�˰��i�H���T����, �٦b��s��.
                �O�_�� 1080p, 1080i ���t�O ?

                �Ҽ{�令 AVStream �� pts ������.
            */

            delay = 2 * av_rescale_q( frame->pts,  dec_ctx->time_base, avr );
            //delay = av_rescale_q( frame->pts, dec_ctx->time_base, avr );

            //if (delay > 0 && delay < 1000000)
              //  usleep(delay);
        }
        //last_pts = frame->pts;
    }

    ts = delay;


    static int64_t fff_ccc = 0;                       
    const double c24 = 24000.0, c1001 = 1001.0, ddd = 1000; // ms
    ts = ( ddd  * c1001 / c24 ) * fff_ccc;
    fff_ccc++;



    int num = dec_ctx->time_base.num;
    int den = dec_ctx->time_base.den;



    /*if( ts == 0 )
        printf("Test");
    else
        printf("test");*/

    //ts = 1.f * frame->pts * av_q2d(avr);

    //printf(" ts = %lld\n", ts );

    return ts;
}
