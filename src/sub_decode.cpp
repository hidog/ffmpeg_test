#include "sub_decode.h"
#include "tool.h"
#include <sstream>


extern "C" {

#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <libavfilter/buffersrc.h>  // use for subtitle
#include <libavfilter/buffersink.h>

} // end extern "C"






/*******************************************************************************
SubDecode::SubDecode()
********************************************************************************/
SubDecode::SubDecode()
    :   Decode()
{
    type  =   AVMEDIA_TYPE_SUBTITLE; 
}









/*******************************************************************************
SubDecode::get_subtitle_param()
********************************************************************************/
std::pair<std::string,std::string>  SubDecode::get_subtitle_param( AVFormatContext* fmt_ctx, std::string src_file, SubData sd )
{
    std::stringstream   ss;
    std::string     in_param, out_param;;
    
    AVRational  frame_rate  =   av_guess_frame_rate( fmt_ctx, fmt_ctx->streams[sd.video_index], NULL );

    int     sar_num     =   fmt_ctx->streams[sd.video_index]->codecpar->sample_aspect_ratio.num; // old code use fmt_ctx->streams[sd.video_index]->sample_aspect_ratio.num
    int     sar_den     =   FFMAX( fmt_ctx->streams[sd.video_index]->codecpar->sample_aspect_ratio.den, 1 );

    int     tb_num      =   fmt_ctx->streams[sd.video_index]->time_base.num;
    int     tb_den      =   fmt_ctx->streams[sd.video_index]->time_base.den;

    ss << "video_size=" << sd.width << "x" << sd.height << ":pix_fmt=" << static_cast<int>(sd.pix_fmt) 
       << ":time_base=" << tb_num << "/" << tb_den << ":pixel_aspect=" << sar_num << "/" << sar_den;

    if( frame_rate.num != 0 && frame_rate.den != 0 )
        ss << ":frame_rate=" << frame_rate.num << "/" << frame_rate.den;

    in_param   =   ss.str();

    MYLOG( LOG::INFO, "in = %s", in_param.c_str() );

    ss.str("");
    ss.clear();   

    if( is_graphic_subtitle() == false )  // 無法下參數. 圖片subtitle.
    {
        // make filename param. 留意絕對路徑的格式, 不能亂改, 會造成錯誤.
        std::string     filename_param  =   "\\";
        filename_param  +=  src_file;
        filename_param.insert( 2, 1, '\\' );

        // 理論上這邊的字串可以精簡...
        sub_index   =   sd.sub_index;
        ss << "subtitles='" << filename_param << "':stream_index=" << sub_index;

        out_param    =   ss.str();

        MYLOG( LOG::INFO, "out = %s", out_param.c_str() );
    }

    return  std::make_pair( in_param, out_param );
}






/*******************************************************************************
SubDecode::set_filter_args()
********************************************************************************/
void    SubDecode::set_filter_args( std::string args )
{
    subtitle_args   =   args;
}





/*******************************************************************************
SubDecode::output_decode_info()
********************************************************************************/
void    SubDecode::output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx )
{
    MYLOG( LOG::INFO, "sub dec name = %s", dec->name );
    MYLOG( LOG::INFO, "sub dec long name = %s", dec->long_name );
    MYLOG( LOG::INFO, "sub dec codec id = %s", avcodec_get_name(dec->id) );
    //MYLOG( LOG::INFO, "audio bitrate = %d", dec_ctx->bit_rate );
}




/*******************************************************************************
SubDecode::~SubDecode()
********************************************************************************/
SubDecode::~SubDecode()
{
    end();
}





/*******************************************************************************
SubDecode::open_codec_context()
********************************************************************************/
int     SubDecode::open_codec_context( AVFormatContext *fmt_ctx )
{
    Decode::open_all_codec( fmt_ctx, type );

    if( dec_ctx->pix_fmt != AV_PIX_FMT_NONE )
        is_graphic  =   true;
    else
        is_graphic  =   false;

    return  SUCCESS;
}





/*******************************************************************************
SubDecode::is_graphic_subtitle()
********************************************************************************/
bool    SubDecode::is_graphic_subtitle()
{
    return  is_graphic;
}



/*******************************************************************************
SubDecode::init()
********************************************************************************/
int     SubDecode::init()
{
    got_sub         =   0;
    sub_dpts        =   -1; 
    sub_duration    =   -1;

    //graph    =   avfilter_graph_alloc();
    Decode::init();
    return  SUCCESS;
}






/*******************************************************************************
SubDecode::send_video_frame()
********************************************************************************/
int SubDecode::send_video_frame( AVFrame *video_frame )
{
    //frame->best_effort_timestamp    =   video_frame->best_effort_timestamp;
    //frame->pts                      =   video_frame->best_effort_timestamp;

    //int ret =   av_buffersrc_add_frame_flags( bf_src_ctx, video_frame, AV_BUFFERSRC_FLAG_KEEP_REF );    
    int ret =   av_buffersrc_add_frame( bf_src_ctx, video_frame );    

    if( ret < 0 )    
        MYLOG( LOG::ERROR, "add frame flag fail." );
    return  ret;
}





/*******************************************************************************
SubDecode::init_sub_image()
********************************************************************************/
void    SubDecode::init_sub_image( SubData sd )
{
    sub_image  =   QImage{ sd.width, sd.height, QImage::Format_RGB888 };  // 未來考慮移走這邊的code, 避免重複初始化.
}





/*******************************************************************************
SubDecode::init_sws_ctx()
********************************************************************************/
int SubDecode::init_sws_ctx( SubData sd )
{
    sub_dst_bufsize   =   av_image_alloc( sub_dst_data, sub_dst_linesize, sd.width, sd.height, AV_PIX_FMT_RGB24, 1 );
    if( sub_dst_bufsize < 0 )
    {
        MYLOG( LOG::ERROR, "Could not allocate subtitle image buffer" );
        return  ERROR;
    }

    AVPixelFormat   pix_fmt     =   static_cast<AVPixelFormat>(sd.pix_fmt);
    sws_ctx     =   sws_getContext( sd.width, sd.height, pix_fmt, sd.width, sd.height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL );   
    return  SUCCESS;
}






/*******************************************************************************
SubDecode::render_subtitle()
********************************************************************************/
int SubDecode::render_subtitle()
{
    //int ret     =   av_buffersink_get_frame( bf_sink_ctx, frame );    
    int ret     =   av_buffersink_get_frame_flags( bf_sink_ctx, frame, 0 );    

    if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ) 
        return  0;  // 沒資料,但沒錯誤.
    else if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "get frame fail." );
        return  -1;
    }

    ret     =   sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, sub_dst_data, sub_dst_linesize );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "ret = %d", ret );

    memcpy( sub_image.bits(), sub_dst_data[0], sub_dst_bufsize );

    return 1;
}





/*******************************************************************************
SubDecode::get_sub_src_type()
********************************************************************************/
SubSourceType   SubDecode::get_sub_src_type()
{
    return  sub_src_type;
}




/*******************************************************************************
SubDecode::set_sub_src_type()
********************************************************************************/
void    SubDecode::set_sub_src_type( SubSourceType type )
{
    sub_src_type    =   type;
}





/*******************************************************************************
SubDecode::end()
********************************************************************************/
int     SubDecode::end()
{
    if( bf_src_ctx != nullptr )
    {
        avfilter_free( bf_src_ctx );
        bf_src_ctx  =   nullptr;
    }

    if( bf_sink_ctx != nullptr )
    {
        avfilter_free( bf_sink_ctx );
        bf_sink_ctx     =   nullptr;
    }

    if( sws_ctx != nullptr )
    {
        sws_freeContext( sws_ctx );
        sws_ctx     =   nullptr;
    }

    if( graph != nullptr )
    {
        avfilter_graph_free(&graph);
        graph    =   nullptr;
    }

    sub_file.clear();
    subtitle_args.clear();
    sub_src_type    =   SubSourceType::NONE;
    is_graphic      =   false;

    got_sub         =   0;
    sub_dpts        =   -1; 
    sub_duration    =   -1;

    Decode::end();
    return  SUCCESS;
}





/*******************************************************************************
SubDecode::set_subfile()
********************************************************************************/
void    SubDecode::set_subfile( std::string path )
{
    sub_file    =   path;
}







/*******************************************************************************
SubDecode::output_audio_frame_info()
********************************************************************************/
void    SubDecode::output_sub_frame_info()
{
    /*char    buf[AV_TS_MAX_STRING_SIZE]{0};
    int     per_sample  =   av_get_bytes_per_sample( static_cast<AVSampleFormat>(frame->format) );
    auto    pts_str     =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    MYLOG( LOG::INFO, "audio_frame n = %d, nb_samples = %d, pts : %s", frame_count++, frame->nb_samples, pts_str );*/
}







/*******************************************************************************
SubDecode::open_subtitle_filter()
********************************************************************************/
bool SubDecode::open_subtitle_filter( std::string args, std::string desc )
{
    // release and re create
    if( bf_src_ctx != nullptr )
    {
        avfilter_free( bf_src_ctx );
        bf_src_ctx  =   nullptr;
    }

    if( bf_sink_ctx != nullptr )
    {
        avfilter_free( bf_sink_ctx );
        bf_sink_ctx     =   nullptr;
    }

    if( graph != nullptr )
    {
        avfilter_graph_free(&graph);
        graph   =   nullptr;
    }

    //
    int     ret     =   0;

    const AVFilter  *buffersrc      =   avfilter_get_by_name("buffer");
    const AVFilter  *buffersink     =   avfilter_get_by_name("buffersink");
    AVFilterInOut   *output         =   avfilter_inout_alloc();
    AVFilterInOut   *input          =   avfilter_inout_alloc();

    // lambda operator, 省略了傳入參數括號. 
    auto release    =   [ &output, &input ]
    {
        avfilter_inout_free( &output );
        avfilter_inout_free( &input );
    };

    graph    =   avfilter_graph_alloc();    
    if( output == nullptr || input == nullptr || graph == nullptr ) 
    {
        MYLOG( LOG::ERROR, "alloc fail." );
        release();
        return false;
    }

    // Crear filtro de entrada, necesita arg
    ret     =   avfilter_graph_create_filter( &bf_src_ctx, buffersrc, "in", args.c_str(), nullptr, graph );
    if( ret < 0 ) 
    {
        release();
        MYLOG( LOG::ERROR, "avfilter_graph_create_filter error" );
        return false;
    }

    ret     =   avfilter_graph_create_filter( &bf_sink_ctx, buffersink, "out", nullptr, nullptr, graph );
    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "avfilter_graph_create_filter error" );
        release();
        return false;
    }

    if( desc.empty() == false )
    {        
        output->name        =   av_strdup("in");
        output->next        =   nullptr;
        output->pad_idx     =   0;
        output->filter_ctx  =   bf_src_ctx;

        input->name         =   av_strdup("out");
        input->next         =   nullptr;
        input->pad_idx      =   0;
        input->filter_ctx   =   bf_sink_ctx;

        //
        ret     =   avfilter_graph_parse_ptr( graph, desc.c_str(), &input, &output, nullptr );
        if( ret < 0 )
        {
            MYLOG( LOG::ERROR, "avfilter_graph_parse_ptr error" );
            release();
            return false;
        }
    }
    else
    {
        // 這段code其實用不到 input, output, 但就先不特別做處理了
        ret     =   avfilter_link( bf_src_ctx, 0, bf_sink_ctx, 0 );
        if( ret < 0 )
        {
            MYLOG( LOG::ERROR, "avfilter_link error" );
            release();
            return false;
        }
    }


    ret     =   avfilter_graph_config( graph, nullptr );
    if( ret < 0 )
    {
        MYLOG( LOG::DEBUG, "avfilter_graph_config error" );
        release();
        return false;
    }

    char *str   =   avfilter_graph_dump( graph, nullptr );
    MYLOG( LOG::DEBUG, "options = %s", str );

    release();
    return true;
}







/*******************************************************************************
SubDecode::generate_subtitle_image()

這個程式碼沒執行過, 是從網路複製過來的, 想辦法測試.
********************************************************************************/
void    SubDecode::generate_subtitle_image( AVSubtitle &subtitle )
{
    MYLOG( LOG::DEBUG, "first run generate_subtitle_image subtitle" );

    int     i;

    for( i = 0; i < subtitle.num_rects; i++ )
    {
        AVSubtitleRect  *sub_rect   =   subtitle.rects[i];

        int     dst_linesize[4];
        uint8_t *dst_data[4];

        //
        av_image_alloc( dst_data, dst_linesize, sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA, 1 );

        SwsContext *swsContext  =   sws_getContext( sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
                                                    sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA,
                                                    SWS_BILINEAR, nullptr, nullptr, nullptr );

        sws_scale( swsContext, sub_rect->data, sub_rect->linesize, 0, sub_rect->h, dst_data, dst_linesize );
        sws_freeContext(swsContext);        

        sub_image  =   QImage( dst_data[0], sub_rect->w, sub_rect->h, QImage::Format_RGBA8888).copy();
        sub_image.save("H:\\test.jpg");
        av_freep(&dst_data[0]);
    }
}






/*******************************************************************************
SubDecode::is_video_in_duration()
********************************************************************************/
bool    SubDecode::is_video_in_duration( int64_t timestamp )
{
    if( timestamp >= sub_dpts && timestamp <= sub_dpts + sub_duration )
        return  true;
    else
        return  false;
}





/*******************************************************************************
SubDecode::decode_subtitle()
********************************************************************************/
int    SubDecode::decode_subtitle( AVPacket* pkt )
{
    AVCodecContext  *dec    =   pkt == nullptr ? dec_map[cs_index] : dec_map[pkt->stream_index];

    AVSubtitle  subtitle {0};

    got_sub     =   0;
    int     ret         =   avcodec_decode_subtitle2( dec, &subtitle, &got_sub, pkt );
    
    if( ret >= 0 && got_sub > 0 )
    {
        if( got_sub > 0 )
        {
            subtitle.pts    =   pkt->pts;       // 時間控制項怪怪的,需要研究

            // 代表字幕是圖片格式, 需要產生對應的字幕圖檔.
            if( subtitle.format == 0 )
            {
                generate_subtitle_image( subtitle );

                if( subtitle.pts != AV_NOPTS_VALUE)
                    sub_dpts    =   1.0 * subtitle.pts / AV_TIME_BASE;
                else
                    sub_dpts    =   0;                

                sub_duration    =   1.0 * (subtitle.end_display_time - subtitle.start_display_time) / 1000;

                if( subtitle.start_display_time != 0 )
                    MYLOG( LOG::ERROR, "start time not zero, need handle." );
            }

            avsubtitle_free( &subtitle );
            return  SUCCESS;
        }
        else         
            return  SUCCESS;  // decode success, but no subtitle.
    }
    else
    {
        MYLOG( LOG::ERROR, "decode subtitle fail" );
        return  ERROR;
    }   
    

#if 0
    // 用來輸出訊息的測試程式碼
    qreal pts = pkt->pts * av_q2d(subStream->time_base);
    qreal duration = pkt->duration * av_q2d(subStream->time_base);

    // https://tsduck.io/doxy/namespacets.html
    // 可以用 ts 套件做文字轉換.
    const char *text = const_int8_ptr(pkt->data);
    MYLOG( LOG::DEBUG, "pts = %lf, duration = %lf, text = %s", pts, duration, pkt->data );
#endif

#if 0
    // 用來輸出訊息的測試程式碼
    AVSubtitleRect **rects = subtitle.rects;
    for (int i = 0; i < subtitle.num_rects; i++) 
    {
        AVSubtitleRect rect = *rects[i];
        if (rect.type == SUBTITLE_ASS)                 
            MYLOG( LOG::DEBUG, "ASS %s", rect.ass)                
        else if (rect.x == SUBTITLE_TEXT)                 
            MYLOG( LOG::DEBUG, "TEXT %s", rect.text)
    }
#endif
}







/*******************************************************************************
SubDecode::get_subtitle_image()
********************************************************************************/
QImage  SubDecode::get_subtitle_image()
{
    return  sub_image;
}





/*******************************************************************************
SubDecode::get_subtitle_image()
********************************************************************************/
bool    SubDecode::exist_stream()
{
    if( sub_file.empty() == false )
        return  true;

    return  Decode::exist_stream();
}



/*******************************************************************************
SubDecode::get_subfile()
********************************************************************************/
std::string     SubDecode::get_subfile()
{
    return  sub_file;
}






/*******************************************************************************
SubDecode::get_embedded_subtitle_list().
********************************************************************************/
std::vector<std::string>    SubDecode::get_embedded_subtitle_list()
{
    std::vector<std::string>    list;

    char    *buf    =   nullptr;

    av_dict_get_string( stream->metadata, &buf, '=', ',' );
    MYLOG( LOG::INFO, "buf = %s\n", buf );

    for( auto itr : stream_map )
    {
        AVDictionaryEntry   *dic   =   av_dict_get( (const AVDictionary*)itr.second->metadata, "title", NULL, AV_DICT_MATCH_CASE );
        if( dic != nullptr )
        {
            MYLOG( LOG::DEBUG, "title %s", dic->value );
            list.emplace_back( std::string(dic->value) );
        }
        else
            list.emplace_back( std::string("default") );  // 遇到多字幕都沒有定義 title 再來調整這裡的程式碼...
    }

    return  list;
}






/*******************************************************************************
SubDecode::sub_info()

https://www.jianshu.com/p/89f2da631e16

實際上不必解開多個字幕軌, 有空再研究.
********************************************************************************/
int     SubDecode::sub_info()
{  
    for( auto itr : stream_map )
    {
        AVDictionaryEntry   *dic   =   av_dict_get( (const AVDictionary*)itr.second->metadata, "title", NULL, AV_DICT_MATCH_CASE );
        MYLOG( LOG::DEBUG, "title %s", dic->value );
    }

#if 0
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
SubDecode::switch_subtltle()
********************************************************************************/
void    SubDecode::switch_subtltle( std::string path )
{
    sub_file    =   path;

    std::string     filename    =   "\\";    
    filename    +=  sub_file;
    filename.insert( 2, 1, '\\' );

    std::stringstream   ss;
    ss << "subtitles='" << filename << "':stream_index=" << 0;

    std::string     desc    =   ss.str();

    open_subtitle_filter( subtitle_args, desc );
}





/*******************************************************************************
SubDecode::switch_subtltle()
********************************************************************************/
void    SubDecode::switch_subtltle( int index )
{
    std::string     desc;

    if( is_graphic_subtitle() == true )    
        open_subtitle_filter( subtitle_args, desc );   // 這邊的例外處理沒有寫得很好,有空再想怎麼修
    else
    {
        sub_index   =   index;

        std::string     filename    =   "\\";    
        filename    +=  sub_file;
        filename.insert( 2, 1, '\\' );

        std::stringstream   ss;
        ss << "subtitles='" << filename << "':stream_index=" << sub_index;

        desc    =   ss.str();

        open_subtitle_filter( subtitle_args, desc );
    }
}







/*******************************************************************************
SubDecode::get_timestamp()
********************************************************************************/
int64_t     SubDecode::get_timestamp()
{
    if( frame->best_effort_timestamp == AV_NOPTS_VALUE )
        return  0;

    double  dpts    =   av_q2d(stream->time_base) * frame->best_effort_timestamp;
    int64_t ts      =   dpts * 1000;  // ms
    return  ts;
}


