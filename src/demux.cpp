#include "demux.h"

#include "tool.h"
#include <sstream>

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
{
    v_codec_id  =   AV_CODEC_ID_NONE;
    a_codec_id  =   AV_CODEC_ID_NONE;
}




/*******************************************************************************
Demux::~Demux()
********************************************************************************/
Demux::~Demux()
{}




#if 0
/*******************************************************************************
Demux::get_video_width()
********************************************************************************/
int     Demux::get_video_width()
{
    return  width;
}
#endif


#if 0
/*******************************************************************************
Demux::get_video_height()
********************************************************************************/
int     Demux::get_video_height()
{
    return  height;
}
#endif




#ifdef USE_MT
/*******************************************************************************
Demux::collect_packet()
********************************************************************************/
void    Demux::collect_packet( AVPacket *_pkt )
{
    std::lock_guard<std::mutex>     lock(pkt_mtx);
    av_packet_unref(_pkt);
    pkt_queue.push(_pkt);
}
#endif





/*******************************************************************************
Demux::init()
********************************************************************************/
int    Demux::init()
{
    int     ret, i;

    /*
        use for multi-thread
    */
#ifdef USE_MT
    for( i = 0; i < 10; i++ )
    {
        pkt_array[i]    =   av_packet_alloc();
        
        if( pkt_array[i] == nullptr )
        {
            ret     =   AVERROR(ENOMEM);
            MYLOG( LOG::ERROR, "Could not allocate packet. error = %d", ret );
            return  ERROR; 
        }

        pkt_queue.emplace( pkt_array[i] );        
    }
#endif


    /*
        av_init_packet(&pkt);
        如果不是宣告為指標,可以用這個函數來初始化
    */
    pkt     =   av_packet_alloc();
    //pkt_bsf =   av_packet_alloc();

    //if( pkt == nullptr || pkt_bsf == nullptr ) 
    if( pkt == nullptr )
    {
        ret     =   AVERROR(ENOMEM);
        MYLOG( LOG::ERROR, "Could not allocate packet. error = %d", ret );
        return  ERROR;
    }

    return  SUCCESS;
}




/*******************************************************************************
Demux::sub_info()

https://www.jianshu.com/p/89f2da631e16

實際上不必解開多個字幕軌, 有空再研究.
********************************************************************************/
int     Demux::sub_info()
{  
#if 0
    // 這邊需要改成loop, 判斷有幾個音軌,並且呈現在UI上.
    ss_idx  =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, NULL, 0 );
    if( ss_idx < 0 )
    {
        MYLOG( LOG::INFO, "no subtitle stream" );        
        return  SUCCESS;
    }
    
    //
    AVStream    *sub_stream   =   fmt_ctx->streams[ss_idx];
    if( sub_stream == nullptr )
    {
        MYLOG( LOG::INFO, "this stream has no sub stream" );
        return  SUCCESS;
    }
    
    //
    AVCodecID   codec_id    =   fmt_ctx->streams[ss_idx]->codecpar->codec_id;
    MYLOG( LOG::INFO, "code name = %s", avcodec_get_name(codec_id) );
    
    // 測試用 未來需要能掃描 metadata, 並且秀出對應的 sub title, audio title.
    AVDictionaryEntry   *dic   =   av_dict_get( (const AVDictionary*)fmt_ctx->streams[ss_idx]->metadata, "title", NULL, AV_DICT_MATCH_CASE );
    MYLOG( LOG::DEBUG, "title %s", dic->value );

    return  ss_idx;
#endif
    return 0;
}




/*******************************************************************************
Demux::video_info()

NOTE: 假設影片只有一個視訊軌,先不處理多重視訊軌的問題.
********************************************************************************/
int     Demux::video_info()
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
#endif

    return SUCCESS;
}






/*******************************************************************************
Demux::audio_info()

遇到的時候再來處理多重音軌的問題
********************************************************************************/
int     Demux::audio_info()
{
#if 0
    as_idx      =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0 );

    //
    AVStream    *audio_stream   =   fmt_ctx->streams[as_idx];
    if( audio_stream == nullptr )
    {
        MYLOG( LOG::INFO, "this stream has no audio stream" );
        return  SUCCESS;
    }
    
    //
    a_codec_id   =   fmt_ctx->streams[as_idx]->codecpar->codec_id;
    MYLOG( LOG::INFO, "code name = %s", avcodec_get_name(a_codec_id) );

    //
    channel     =   fmt_ctx->streams[as_idx]->codecpar->channels;
    sample_rate =   fmt_ctx->streams[as_idx]->codecpar->sample_rate;
    MYLOG( LOG::INFO, "channel = %d, sample rate = %d", channel, sample_rate );

    //
    double a_dur_ms = av_q2d( fmt_ctx->streams[as_idx]->time_base) * fmt_ctx->streams[as_idx]->duration;
    MYLOG( LOG::INFO, "frame duration = %lf ms", a_dur_ms );

#endif
    return  SUCCESS;
}




#if 0
/*******************************************************************************
Demux::get_audio_channel()
********************************************************************************/
int     Demux::get_audio_channel()
{
    return  channel;
}
#endif


#if 0
/*******************************************************************************
Demux::get_audio_sample_rate()
********************************************************************************/
int     Demux::get_audio_sample_rate()
{
    return  sample_rate;
}
#endif



/*******************************************************************************
Demux::stream_info()
********************************************************************************/
int     Demux::stream_info()
{
    int ret =   0;

    //
    ret     =   avformat_find_stream_info( fmt_ctx, nullptr );
    if( ret < 0) 
    {
        MYLOG( LOG::ERROR, "Could not find stream information. ret = %d", ret );
        return  ERROR;
    }

    MYLOG( LOG::INFO, "nb_streams = %d", fmt_ctx->nb_streams );


    /*int vc = 0, ac = 0, sc = 0;
    for( int i = 0; i < fmt_ctx->nb_streams; i++ )
    {
        if( fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
            vc++;
        else if( fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO )
            ac++;
        else if( fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE )
            sc++;
    }
    if( vc > 1 || ac > 1 )    
        MYLOG( LOG::ERROR, "multi video or audio."); // 未來需要處理多重音軌/視訊軌的問題. 理論上不太會遇到多重視訊軌.   */

    // 底下的動作目前無實質作用, 但如果要做 NVidia, 可以考慮在底下函數做判斷.
    //video_info();
    //audio_info();
    //sub_info();

    /* dump input information to stderr */
    //av_dump_format( fmt_ctx, 0, src_file.c_str(), 0 );

    return  SUCCESS;
}















/*******************************************************************************
Demux::end()
********************************************************************************/
int     Demux::end()
{
    int     i;

    avformat_close_input( &fmt_ctx );
    av_packet_free( &pkt );
    //av_bsf_free( &v_bsf_ctx );

    //
#ifdef USE_MT
    while( pkt_queue.empty() == false )
        pkt_queue.pop();
    for( i = 0; i < 10; i++ )
        av_packet_free( &pkt_array[i] );
#endif

    src_file    =   "";

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

    MYLOG( LOG::INFO, "load file %s", src_file.c_str() );
    ret     =   avformat_open_input( &fmt_ctx, src_file.c_str(), NULL, NULL );

    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "Could not open source file %s", src_file.c_str() );
        return  ERROR;
    }

    // 解開video, audio, subtitle info.
    ret     =   stream_info();
    if( ret == ERROR )
    {
        MYLOG( LOG::ERROR, "init fail. ret = %d", ret );
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




#ifdef USE_MT
/*******************************************************************************
Demux::demux_multi_thread()
********************************************************************************/
std::pair<int,AVPacket*>     Demux::demux_multi_thread()
{
    int     ret;
    AVPacket    *packet     =   nullptr;

    if( pkt_queue.empty() == true )
    {
        MYLOG( LOG::WARN, "pkt stack empty." );
        return  std::make_pair( 0, nullptr );
    }

    if( pkt_queue.size() < 10 )
        MYLOG( LOG::DEBUG, "queue size = %d", pkt_queue.size() );

    packet  =   pkt_queue.front();
    pkt_queue.pop();

    ret     =   av_read_frame( fmt_ctx, packet );

    if( ret < 0 )    
        MYLOG( LOG::INFO, "load file end." );

    std::pair<int,AVPacket*>    result  =   std::make_pair( ret, packet );

    return  result;
}
#endif



/*******************************************************************************
Demux::demux()
********************************************************************************/
int    Demux::demux()
{
    int     ret;
    ret     =   av_read_frame( fmt_ctx, pkt );

    if( ret < 0 )    
        MYLOG( LOG::INFO, "load file end." );

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



