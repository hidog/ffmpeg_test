#include "video_decode.h"

#include "sub_decode.h"
#include "video_decode_hw.h"

#include <QPainter>
#include <QImage>

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
    :   Decode(AVMEDIA_TYPE_VIDEO)
{
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
{
    end();
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

    MYLOG( LOG::L_INFO, "width = %d, height = %d, pix_fmt = %d\n", width, height, pix_fmt );
    
    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_RGB24, 1 );
    //video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, width, height, AV_PIX_FMT_YUV420P, 1 );  // when use output_jpg_by_openCV, use this.

    if( video_dst_bufsize < 0 )
    {
        MYLOG( LOG::L_ERROR, "Could not allocate raw video buffer" );
        return  R_ERROR;
    }

    // NOTE : 可以改變寬高. 
    sws_ctx     =   sws_getContext( width, height, pix_fmt,                     // src
                                    width, height, AV_PIX_FMT_RGB24,            // dst
                                    SWS_BICUBIC, NULL, NULL, NULL);                        

    if( sws_ctx == nullptr )
    {
        MYLOG( LOG::L_ERROR, "sws init fail." );
    }

#ifdef FFMPEG_TEST
    output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_QT, this );
    //output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_openCV, this );
    //output_frame_func   =   std::bind( &VideoDecode::test_image_process, this );
#endif

    Decode::init();

    // 理論上可以在這之前就設置好 sub_dec, 但目前規劃是 init 後再設置 sub_dec.
    if( sub_dec != nullptr )
        MYLOG( LOG::L_ERROR, "sub_dec not null." );

    return  R_SUCCESS;
}






/*******************************************************************************
VideoDecode::set_subtitle_decoder()
********************************************************************************/
void    VideoDecode::set_subtitle_decoder( SubDecode* sd )
{
    if( sd == nullptr )
        MYLOG( LOG::L_ERROR, "sd is null." );
    if( sub_dec != nullptr )
        MYLOG( LOG::L_ERROR, "sub_dec is not null." );

    sub_dec     =   sd;

#ifdef FFMPEG_TEST
    if( output_frame_func != nullptr )
        output_frame_func   =   nullptr;

    if( sub_dec->is_graphic_subtitle() == false )
        output_frame_func   =   std::bind( &SubDecode::output_jpg_by_QT, sub_dec );
    else
        output_frame_func   =   std::bind( &VideoDecode::output_overlay_by_QT, this );
#endif
}






/*******************************************************************************
VideoDecode::output_decode_info()
********************************************************************************/
void    VideoDecode::output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx )
{
    MYLOG( LOG::L_INFO, "video dec name = %s", dec->name );
    MYLOG( LOG::L_INFO, "video dec long name = %s", dec->long_name );
    MYLOG( LOG::L_INFO, "video dec codec id = %s", avcodec_get_name(dec->id) );
    MYLOG( LOG::L_INFO, "video bitrate = %lld, pix_fmt = %s", dec_ctx->bit_rate, av_get_pix_fmt_name(dec_ctx->pix_fmt) );
}








/*******************************************************************************
VideoDecode::output_video_frame_info()
********************************************************************************/
void    VideoDecode::output_video_frame_info()
{
    const char  *frame_format_str   =   av_get_pix_fmt_name( static_cast<AVPixelFormat>(frame->format) );

    MYLOG( LOG::L_INFO, "frame width = %d, height = %d", frame->width, frame->height );
    MYLOG( LOG::L_INFO, "frame format = %s", frame_format_str );

    char        buf[AV_TS_MAX_STRING_SIZE]{0};
    const char* time_str    =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    MYLOG( LOG::L_INFO, "video_frame = %d, coded_n : %d, time = %s", frame_count, frame->coded_picture_number, time_str );
}





/*******************************************************************************
VideoDecode::end()
********************************************************************************/
int     VideoDecode::end()
{
    if( frame_count > 0 && dec_ctx != nullptr )
    {
        int     num     =   stream->avg_frame_rate.num == 0 ? 1 : stream->avg_frame_rate.num;
        MYLOG( LOG::L_INFO, "video decode %d frames.", frame_count );  // 實際上是 frame_count + 1 張
        int64_t     duration_time   =   1000LL * frame_count * stream->avg_frame_rate.den / num; // ms
        int64_t     ms              =   duration_time % 1000;
        int64_t     sec             =   duration_time / 1000 % 60;
        int64_t     minute          =   duration_time / 1000 / 60 % 60;
        int64_t     hour            =   duration_time / 1000 / 60 / 60;
        MYLOG( LOG::L_INFO, "video decode duration = [ %2lld : %2lld : %2lld . %3lld", hour, minute, sec, ms );
    }

    //
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

    sub_dec =   nullptr; // 非 owner, 不能刪除記憶體

    Decode::end();
    return  R_SUCCESS;
}





/*******************************************************************************
VideoDecode::generate_overlay_image()
********************************************************************************/
void    VideoDecode::generate_overlay_image()
{
    overlay_image   =   QImage{ width, height, QImage::Format_RGB888 }; 
    sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, video_dst_data, video_dst_linesize );
    memcpy( overlay_image.bits(), video_dst_data[0], video_dst_bufsize );
}






/*******************************************************************************
VideoDecode::get_video_image()
********************************************************************************/
QImage      VideoDecode::get_video_image()
{
    QImage  image { width, height, QImage::Format_RGB888 };

    //av_image_fill_linesizes( linesizes, AV_PIX_FMT_RGB24, frame->width );
    sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, video_dst_data, video_dst_linesize );
    memcpy( image.bits(), video_dst_data[0], video_dst_bufsize );

    return  image;
}





/*******************************************************************************
VideoDecode::output_video_data()

某些影片不能直接複製到QImage的記憶體,需要先複製到ffmpeg create的video_dst_data.
********************************************************************************/
VideoData   VideoDecode::output_video_data()
{
    VideoData   vd;
    vd.index        =   frame_count;
    vd.timestamp    =   get_timestamp();

    if( sub_dec == nullptr )        
        vd.frame        =   get_video_image();
    else
    {
        if( sub_dec->is_graphic_subtitle() == false )        
            vd.frame    =   sub_dec->get_subtitle_image();        
        else
            vd.frame    =   overlay_image;
    }

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
    if( frame_pts == AV_NOPTS_VALUE )
        return  0;

    double  dpts    =   av_q2d(stream->time_base) * frame_pts;
    int64_t ts      =   dpts * 1000;  // ms
    return  ts;
}





/*******************************************************************************
VideoDecode::get_frame()
********************************************************************************/
AVFrame*    VideoDecode::get_frame()
{
    if( sub_dec == nullptr )
        return  Decode::get_frame();
    else
        return  sub_dec->get_frame();
}




/*******************************************************************************
VideoDecode::get_pts()
逆推回 pts.
NOTE: 傳入值單位是 sec. 
********************************************************************************/
int64_t     VideoDecode::get_pts( int sec )
{
    int64_t     pts =   0;
    pts     =   static_cast<int64_t>( 1.0 * sec / av_q2d(stream->time_base) );
    return  pts;
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
        MYLOG( LOG::L_INFO, "this stream has no video stream" );
        return  R_SUCCESS;
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

    MYLOG( LOG::L_INFO, "width = %d, height = %d, depth = %d", width, height, depth );
    MYLOG( LOG::L_INFO, "code name = %s", avcodec_get_name(v_codec_id) );

    //
    double  fps     =   av_q2d( fmt_ctx->streams[vs_idx]->r_frame_rate );
    MYLOG( LOG::L_INFO, "fps = %lf", fps );
#endif

    return R_SUCCESS;
}



/*******************************************************************************
VideoDecode::render_nongraphic_subtitle()
********************************************************************************/
int     VideoDecode::render_nongraphic_subtitle()
{
    int ret =   sub_dec->send_video_frame( frame );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "send video to subtitle fail." );

    ret =   sub_dec->render_subtitle();
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "render subtitle fail." );

#ifdef _DEBUG
     // 理論上一次只有一張, 保險起見加檢查.
     if( sub_dec->resend_to_filter() > 0 )
        MYLOG( LOG::L_ERROR, "multi subtitle frame." );
#endif

     return     ret;
}






/*******************************************************************************
VideoDecode::overlap_subtitle_image()
********************************************************************************/
int     VideoDecode::overlap_subtitle_image()
{
    int64_t     timestamp   =   this->get_timestamp();
    
    generate_overlay_image(); // overlay_image 在這邊產生

    if( sub_dec->is_video_in_duration( timestamp ) == true )    
    {
        QImage  s_img   =   sub_dec->get_subtitle_image();
        
        QPainter    painter( &overlay_image );
        QPoint      pos =   sub_dec->get_subtitle_image_pos();
        painter.drawImage( pos, s_img );

        return  HAVE_FRAME;
    }

    return  HAVE_FRAME;
}








/*******************************************************************************
VideoDecode::recv_frame()
********************************************************************************/
int     VideoDecode::recv_frame( int index )
{
    int ret =   Decode::recv_frame(index);
    
    // exist frame.
    if( ret > 0 )
    {
        frame->pts  =   frame->best_effort_timestamp;  // need set frame pts, otherwise, 不然部分檔案會出現字幕閃爍
        frame_pts   =   frame->best_effort_timestamp;

        if( sub_dec != nullptr )
        {
            if( sub_dec->is_graphic_subtitle() == true )
                ret     =   overlap_subtitle_image();
            else
                ret     =   render_nongraphic_subtitle();
        }
    }

    return  ret;
}





#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecode::flush()
********************************************************************************/
int     VideoDecode::flush()
{
#ifdef RENDER_SUBTITLE
    int ret =   send_packet(nullptr);
    if( ret >= 0 )
    {       
        while(true)
        {
            ret     =   recv_frame(-1);
            if( ret <= 0 )
                break;

            if( output_frame_func != nullptr )
                output_frame_func();
            unref_frame();
        }
    }
#else
    Decode::flush();
#endif

    return  R_SUCCESS;
}
#endif




/*******************************************************************************
VideoDecode::unref_frame()
********************************************************************************/
void    VideoDecode::unref_frame()
{
    if( sub_dec != nullptr )
        sub_dec->unref_frame();
    Decode::unref_frame();
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

    char    str[1000];
    sprintf( str, "%s\\%d.jpg", output_jpg_path.c_str(), frame_count );
    if( frame_count % 100 == 0 )
        MYLOG( LOG::L_DEBUG, "save jpg %s", str );
    img.save(str);

    return  0;
#endif
}
#endif





#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecode::output_overlay_by_QT()
********************************************************************************/
int     VideoDecode::output_overlay_by_QT()
{
    char    str[1000];
    sprintf( str, "%s\\%d.jpg", output_jpg_path.c_str(), frame_count );
    if( frame_count % 100 == 0 )
        MYLOG( LOG::L_DEBUG, "save jpg %s", str );
    overlay_image.save(str);

    return  0;
}
#endif





#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecode::set_output_jpg_path()
********************************************************************************/
void    VideoDecode::set_output_jpg_path( std::string _path )
{
    output_jpg_path    =   _path;
}
#endif




/*******************************************************************************
VideoDecode::is_attached()

代表此 stream 是封面之類的
https://www.twblogs.net/a/5bb526732b717773002dc5a2
********************************************************************************/
bool    VideoDecode::is_attached()
{
    if( (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0 )
        return  true;
    else
        return  false;
}






/*******************************************************************************
get_video_decoder_instance()
********************************************************************************/
VideoDecode*    get_video_decoder_instance()
{
#ifdef CPU_DECODE
    return  new VideoDecode;
#elif defined( HW_DECODE )
    return  new VideoDecodeHW;
#elif defined( NV_DECODE)
    return  new VideoDecodeNV;
#else
    assert(0);
    return  nullptr;
#endif
}
