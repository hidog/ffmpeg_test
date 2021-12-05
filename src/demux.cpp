#include "demux.h"
#include "tool.h"

extern "C" {

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
{
    end();
}







/*******************************************************************************
Demux::init()
********************************************************************************/
int    Demux::init()
{
    int     ret;

    /*
        use for multi-thread
    */
#ifdef USE_MT
    int     i;
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
    if( pkt == nullptr )
    {
        ret     =   AVERROR(ENOMEM);
        MYLOG( LOG::ERROR, "Could not allocate packet. error = %d", ret );
        return  ERROR;
    }

    return  SUCCESS;
}





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

    return  SUCCESS;
}






/*******************************************************************************
Demux::end()
********************************************************************************/
int     Demux::end()
{
    if( fmt_ctx != nullptr )
    {
        avformat_close_input( &fmt_ctx );
        fmt_ctx     =   nullptr;
    }

    if( pkt != nullptr )
    {
        av_packet_free( &pkt );
        pkt         =   nullptr;
    }

    //
#ifdef USE_MT
    while( pkt_queue.empty() == false )
        pkt_queue.pop();
    for( i = 0; i < 10; i++ )
        av_packet_free( &pkt_array[i] );
#endif

    return  SUCCESS;
}






/*******************************************************************************
Demux::open_input()
********************************************************************************/
int     Demux::open_input( std::string src_file )
{
    fmt_ctx     =   avformat_alloc_context();    
    int  ret    =   0;

    MYLOG( LOG::INFO, "load file %s", src_file.c_str() );
    ret     =   avformat_open_input( &fmt_ctx, src_file.c_str(), NULL, NULL );

    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "Could not open source file %s", src_file.c_str() );
        return  ERROR;
    }

    // 
    ret     =   stream_info();
    if( ret == ERROR )
    {
        MYLOG( LOG::ERROR, "init fail. ret = %d", ret );
        return  ERROR;
    }

    // dump input information to stderr 
    av_dump_format( fmt_ctx, 0, src_file.c_str(), 0 );

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
        MYLOG( LOG::INFO, "load file end." );

    return ret;
}







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




#if 0
void av_dump_format(AVFormatContext *ic, int index,
    const char *url, int is_output)
{
    int i;
    uint8_t *printed = ic->nb_streams ? av_mallocz(ic->nb_streams) : NULL;
    if (ic->nb_streams && !printed)
        return;

    av_log(NULL, AV_LOG_INFO, "%s #%d, %s, %s '%s':\n",
        is_output ? "Output" : "Input",
        index,
        is_output ? ic->oformat->name : ic->iformat->name,
        is_output ? "to" : "from", url);
    dump_metadata(NULL, ic->metadata, "  ");

    if (!is_output) {
        av_log(NULL, AV_LOG_INFO, "  Duration: ");
        if (ic->duration != AV_NOPTS_VALUE) {
            int64_t hours, mins, secs, us;
            int64_t duration = ic->duration + (ic->duration <= INT64_MAX - 5000 ? 5000 : 0);
            secs  = duration / AV_TIME_BASE;
            us    = duration % AV_TIME_BASE;
            mins  = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
            av_log(NULL, AV_LOG_INFO, "%02"PRId64":%02"PRId64":%02"PRId64".%02"PRId64"", hours, mins, secs,
                (100 * us) / AV_TIME_BASE);
        } else {
            av_log(NULL, AV_LOG_INFO, "N/A");
        }
        if (ic->start_time != AV_NOPTS_VALUE) {
            int secs, us;
            av_log(NULL, AV_LOG_INFO, ", start: ");
            secs = llabs(ic->start_time / AV_TIME_BASE);
            us   = llabs(ic->start_time % AV_TIME_BASE);
            av_log(NULL, AV_LOG_INFO, "%s%d.%06d",
                ic->start_time >= 0 ? "" : "-",
                secs,
                (int) av_rescale(us, 1000000, AV_TIME_BASE));
        }
        av_log(NULL, AV_LOG_INFO, ", bitrate: ");
        if (ic->bit_rate)
            av_log(NULL, AV_LOG_INFO, "%"PRId64" kb/s", ic->bit_rate / 1000);
        else
            av_log(NULL, AV_LOG_INFO, "N/A");
        av_log(NULL, AV_LOG_INFO, "\n");
    }

    if (ic->nb_chapters)
        av_log(NULL, AV_LOG_INFO, "  Chapters:\n");
    for (i = 0; i < ic->nb_chapters; i++) {
        const AVChapter *ch = ic->chapters[i];
        av_log(NULL, AV_LOG_INFO, "    Chapter #%d:%d: ", index, i);
        av_log(NULL, AV_LOG_INFO,
            "start %f, ", ch->start * av_q2d(ch->time_base));
        av_log(NULL, AV_LOG_INFO,
            "end %f\n", ch->end * av_q2d(ch->time_base));

        dump_metadata(NULL, ch->metadata, "      ");
    }

    if (ic->nb_programs) {
        int j, k, total = 0;
        for (j = 0; j < ic->nb_programs; j++) {
            const AVProgram *program = ic->programs[j];
            const AVDictionaryEntry *name = av_dict_get(program->metadata,
                "name", NULL, 0);
            av_log(NULL, AV_LOG_INFO, "  Program %d %s\n", program->id,
                name ? name->value : "");
            dump_metadata(NULL, program->metadata, "    ");
            for (k = 0; k < program->nb_stream_indexes; k++) {
                dump_stream_format(ic, program->stream_index[k],
                    index, is_output);
                printed[program->stream_index[k]] = 1;
            }
            total += program->nb_stream_indexes;
        }
        if (total < ic->nb_streams)
            av_log(NULL, AV_LOG_INFO, "  No Program\n");
    }

    for (i = 0; i < ic->nb_streams; i++)
        if (!printed[i])
            dump_stream_format(ic, i, index, is_output);

    av_free(printed);
}
#endif