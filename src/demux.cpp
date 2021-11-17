#include "demux.h"

#include "tool.h"

extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

} // end extern "C"





/*******************************************************************************
Demux::Demux()
********************************************************************************/
Demux::Demux()
{}




/*******************************************************************************
Demux::~Demux()
********************************************************************************/
Demux::~Demux()
{}





/*******************************************************************************
Demux::init()
********************************************************************************/
int    Demux::init()
{
    int ret;

    ret     =   get_stream_info();
    if( ret == ERROR )
    {
        ERRLOG( "init fail. ret = %d", ret );
        return  ERROR;
    }

    /*
        av_init_packet(&pkt);
        �p�G���O�ŧi������,�i�H�γo�Ө�ƨӪ�l��
    */
    pkt     =   av_packet_alloc();
    //pkt_bsf =   av_packet_alloc();

    //if( pkt == nullptr || pkt_bsf == nullptr ) 
    if( pkt == nullptr )
    {
        ret =   AVERROR(ENOMEM);
        ERRLOG( "Could not allocate packet. error = %d", ret );
        return  ERROR;
    }

    return  SUCCESS;
}




/*******************************************************************************
Demux::get_stream_info()
********************************************************************************/
int     Demux::get_video_info()
{
    vs_idx  =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0 );

    //
    AVStream    *video_stream   =   fmt_ctx->streams[vs_idx];
    if( video_stream == nullptr )
        printf("this stream has no video stream\n");


    //AVRational avr = {1, AV_TIME_BASE};
    //auto ttt = fmt_ctx->duration * av_q2d( avr );
    //printf("ttt = %lf\n", ttt);



    //AVCodecID   codec_id    =   fmt_ctx->streams[vs_idx]->codecpar->codec_id;

    int width   =   fmt_ctx->streams[vs_idx]->codecpar->width;
    int height  =   fmt_ctx->streams[vs_idx]->codecpar->height;
    int depth   =   8;
    if( fmt_ctx->streams[vs_idx]->codecpar->format == AV_PIX_FMT_YUV420P10LE )
        depth = 10;
    if( fmt_ctx->streams[vs_idx]->codecpar->format == AV_PIX_FMT_YUV420P12LE )
        depth = 12;

    printf( "width = %d, height = %d, depth = %d\n", width, height, depth );

#if 0
    bool flag1  =   !strcmp( fmt_ctx->iformat->long_name, "QuickTime / MOV" )   ||
                    !strcmp( fmt_ctx->iformat->long_name, "FLV (Flash Video)" ) ||
                    !strcmp( fmt_ctx->iformat->long_name, "Matroska / WebM" );
    bool flag2  =   codec_id == AV_CODEC_ID_H264 || codec_id == AV_CODEC_ID_HEVC;

    use_bsf     =   flag1 && flag2;
#endif

#if 0
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

    return SUCCESS;
}






/*******************************************************************************
Demux::get_audio_info()
********************************************************************************/
int     Demux::get_audio_info()
{
    as_idx      =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0 );
    //AVCodecID codec_id    =   fmt_ctx->streams[as_idx]->codecpar->codec_id;

    AVStream    *audio_stream   =   fmt_ctx->streams[as_idx];
    if( audio_stream == nullptr )
        printf("this stream has no audio stream\n");

    double a_dur_ms = av_q2d( fmt_ctx->streams[as_idx]->time_base) * fmt_ctx->streams[as_idx]->duration;
    printf("frame duration = %lf ms\n", a_dur_ms );


    return  SUCCESS;
}




/*******************************************************************************
Demux::get_video_index()
********************************************************************************/
int     Demux::get_video_index()
{
    return  vs_idx;
}




/*******************************************************************************
Demux::get_audio_index()
********************************************************************************/
int     Demux::get_audio_index()
{
    return  as_idx;
}





/*******************************************************************************
Demux::get_stream_info()
********************************************************************************/
int     Demux::get_stream_info()
{
    int ret =   0;

    ret =   avformat_find_stream_info( fmt_ctx, nullptr );
    if( ret < 0) 
    {
        ERRLOG( "Could not find stream information. ret = %d", ret );
        return ERROR;
    }

    //
    get_video_info();
    get_audio_info();

    /* dump input information to stderr */
    //av_dump_format( fmt_ctx, 0, src_file.c_str(), 0 );

    return SUCCESS;
}





/*******************************************************************************
Demux::end()
********************************************************************************/
int     Demux::end()
{
    avformat_close_input( &fmt_ctx );
    av_packet_free( &pkt );
    //av_bsf_free( &v_bsf_ctx );

    return  SUCCESS;
}






/*******************************************************************************
Demux::open_input()
********************************************************************************/
int     Demux::open_input( std::string str )
{
    fmt_ctx     =   avformat_alloc_context();

    int  ret    =   0;

    src_file    =   str;
    ret         =   avformat_open_input( &fmt_ctx, src_file.c_str(), NULL, NULL );

    if( ret < 0 )
    {
        ERRLOG( "Could not open source file %s", src_file.c_str() );
        return  ERROR;
    }

    return  SUCCESS;
}





/*******************************************************************************
Demux::get_format_context()
********************************************************************************/
AVFormatContext*    Demux::get_format_context()
{
    return  fmt_ctx;
}






/*******************************************************************************
Demux::get_pkt()
********************************************************************************/
AVPacket*   Demux::get_packet()
{
    return  pkt;

    /*if( use_bsf == true && pkt->stream_index == vs_idx )
        return pkt_bsf;
    else
        return  pkt;*/
}



/*******************************************************************************
Demux::unref_pkt()
********************************************************************************/
void    Demux::unref_packet()
{
    av_packet_unref(pkt);
}



/*******************************************************************************
Demux::demux()
********************************************************************************/
int    Demux::demux()
{
    int     ret;

    ret     =   av_read_frame( fmt_ctx, pkt );

    if( ret < 0 )
    {
        printf("load file end.\n");
        //break;
    }

#if 0
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

    return ret;
}



