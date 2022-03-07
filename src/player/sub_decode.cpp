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
    :   Decode(AVMEDIA_TYPE_SUBTITLE)
{}




/*******************************************************************************
SubDecode::get_subtitle_param()
********************************************************************************/
std::pair<std::string,std::string>  SubDecode::get_subtitle_param( AVFormatContext* fmt_ctx, std::string src_file, SubData sd )
{
    if( is_graphic_subtitle() == true )
        MYLOG( LOG::L_ERROR, "cant handle graphic subtitle." );

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

    MYLOG( LOG::L_INFO, "in = %s", in_param.c_str() );

    ss.str("");
    ss.clear();   

    // make filename param. 留意絕對路徑的格式, 不能亂改, 會造成錯誤.
    std::string     filename_param  =   "\\";
    filename_param  +=  src_file;
    filename_param.insert( 2, 1, '\\' );
    
    // 理論上這邊的字串可以精簡...
    sub_index   =   sd.sub_index;
    ss << "subtitles='" << filename_param << "':stream_index=" << sub_index;
    
    out_param    =   ss.str();
    //out_param    =   "subtitles='\\D\\:/code/test.mkv':stream_index=0";

    MYLOG( LOG::L_INFO, "out = %s", out_param.c_str() );
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
SubDecode::send_packet
********************************************************************************/
int     SubDecode::send_packet( AVPacket *pkt )
{
    int ret =   decode_subtitle( pkt );
    return  ret;
}




/*******************************************************************************
SubDecode::recv_frame

subtitle 跟 video, audio 不同, recv 固定無資料.
********************************************************************************/
int     SubDecode::recv_frame( int index )
{
    return  0;
}





/*******************************************************************************
SubDecode::output_decode_info()
********************************************************************************/
void    SubDecode::output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx )
{
    MYLOG( LOG::L_INFO, "sub dec name = %s", dec->name );
    MYLOG( LOG::L_INFO, "sub dec long name = %s", dec->long_name );
    MYLOG( LOG::L_INFO, "sub dec codec id = %s", avcodec_get_name(dec->id) );
}




/*******************************************************************************
SubDecode::~SubDecode()
********************************************************************************/
SubDecode::~SubDecode()
{
    end();
}





#if 0
/*******************************************************************************
SubDecode::open_codec_context()
********************************************************************************/
int     SubDecode::open_codec_context( AVFormatContext *fmt_ctx )
{
    Decode::open_all_codec( fmt_ctx, type );

    if( dec_ctx != nullptr )
    {
        if( dec_ctx->pix_fmt != AV_PIX_FMT_NONE )
            is_graphic  =   true;
        else
            is_graphic  =   false;
    }

    return  R_SUCCESS;
}
#endif




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
    sub_dpts        =   -1; 
    sub_duration    =   -1;
    has_sub_image   =   false;

#ifdef FFMPEG_TEST
    output_frame_func   =   std::bind( &SubDecode::output_jpg_by_QT, this );
#endif

    Decode::init();
    return  R_SUCCESS;
}





/*******************************************************************************
SubDecode::init_graphic_subtitle()
********************************************************************************/
void    SubDecode::init_graphic_subtitle( SubData sd )
{
    video_width     =   sd.width;
    video_height    =   sd.height;
}





/*******************************************************************************
SubDecode::send_video_frame()
********************************************************************************/
int SubDecode::send_video_frame( AVFrame *video_frame )
{
    //int ret =   av_buffersrc_add_frame_flags( bf_src_ctx, video_frame, AV_BUFFERSRC_FLAG_KEEP_REF );    
    int ret =   av_buffersrc_add_frame( bf_src_ctx, video_frame );    

    if( ret < 0 )    
        MYLOG( LOG::L_ERROR, "add frame flag fail." );
    return  ret;
}









/*******************************************************************************
SubDecode::init_sws_ctx()
********************************************************************************/
int SubDecode::init_sws_ctx( SubData sd )
{
    video_width         =   sd.width;
    video_height        =   sd.height;
    sub_dst_bufsize     =   av_image_alloc( sub_dst_data, sub_dst_linesize, video_width, video_height, AV_PIX_FMT_RGB24, 1 );
    if( sub_dst_bufsize < 0 )
    {
        MYLOG( LOG::L_ERROR, "Could not allocate subtitle image buffer" );
        return  R_ERROR;
    }

    AVPixelFormat   pix_fmt     =   static_cast<AVPixelFormat>(sd.pix_fmt);
    sws_ctx     =   sws_getContext( video_width, video_height, pix_fmt, video_width, video_height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL );   
    return  R_SUCCESS;
}



#ifdef _DEBUG
/*******************************************************************************
SubDecode::resend_to_filter()
********************************************************************************/
int     SubDecode::resend_to_filter()
{
    int ret =   av_buffersink_get_frame_flags( bf_sink_ctx, frame, 0 );    
    return  ret;
}
#endif




/*******************************************************************************
SubDecode::render_subtitle()
********************************************************************************/
int     SubDecode::render_subtitle()
{
    sub_image   =   QImage( video_width, video_height, QImage::Format_RGB888 );

    //int ret     =   av_buffersink_get_frame( bf_sink_ctx, frame );    
    int ret     =   av_buffersink_get_frame_flags( bf_sink_ctx, frame, 0 );    

    if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ) 
        return  0;  // 沒資料,但沒錯誤.
    else if( ret < 0 )
    {
        MYLOG( LOG::L_ERROR, "get frame fail." );
        return  -1;
    }

    ret     =   sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, sub_dst_data, sub_dst_linesize );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "ret = %d", ret );

    memcpy( sub_image.bits(), sub_dst_data[0], sub_dst_bufsize );
    frame_count++;

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

    sub_dpts        =   -1; 
    sub_duration    =   -1;
    has_sub_image   =   false;

    Decode::end();
    return  R_SUCCESS;
}





/*******************************************************************************
SubDecode::set_subfile()
********************************************************************************/
void    SubDecode::set_subfile( std::string path )
{
    sub_file    =   path;

#ifdef _WIN32
    // 斜線會影響執行結果.
    for( auto &c : sub_file )
    {
        if( c == '\\' )
            c   =   '/';
    }
#endif
}







/*******************************************************************************
SubDecode::output_audio_frame_info()
********************************************************************************/
void    SubDecode::output_sub_frame_info()
{
    /*char    buf[AV_TS_MAX_STRING_SIZE]{0};
    int     per_sample  =   av_get_bytes_per_sample( static_cast<AVSampleFormat>(frame->format) );
    auto    pts_str     =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    MYLOG( LOG::L_INFO, "audio_frame n = %d, nb_samples = %d, pts : %s", frame_count++, frame->nb_samples, pts_str );*/
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
        MYLOG( LOG::L_ERROR, "alloc fail." );
        release();
        return false;
    }

    // Crear filtro de entrada, necesita arg
    ret     =   avfilter_graph_create_filter( &bf_src_ctx, buffersrc, "in", args.c_str(), nullptr, graph );
    if( ret < 0 ) 
    {
        release();
        MYLOG( LOG::L_ERROR, "avfilter_graph_create_filter error" );
        return false;
    }

    ret     =   avfilter_graph_create_filter( &bf_sink_ctx, buffersink, "out", nullptr, nullptr, graph );
    if( ret < 0 )
    {
        MYLOG( LOG::L_ERROR, "avfilter_graph_create_filter error" );
        release();
        return false;
    }
    
    //
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
        MYLOG( LOG::L_ERROR, "avfilter_graph_parse_ptr error" );
        release();
        return false;
    }

    //
    ret     =   avfilter_graph_config( graph, nullptr );
    if( ret < 0 )
    {
        MYLOG( LOG::L_DEBUG, "avfilter_graph_config error" );
        release();
        return false;
    }

    char *str   =   avfilter_graph_dump( graph, nullptr );
    MYLOG( LOG::L_DEBUG, "options = %s", str );

    release();
    return true;
}







/*******************************************************************************
SubDecode::generate_subtitle_image()
********************************************************************************/
void    SubDecode::generate_subtitle_image( AVSubtitle &subtitle )
{
    // set time stamp.
    if( subtitle.pts != AV_NOPTS_VALUE)
        sub_dpts    =   1000.0 * subtitle.pts / AV_TIME_BASE; // 單位 ms
    else
        sub_dpts    =   0;
    sub_duration    =   1.0 * (subtitle.end_display_time - subtitle.start_display_time) / 1000;  // 單位不明 未來看能不能找到影片測試 end_display_time

    if( subtitle.start_display_time != 0 )
        MYLOG( LOG::L_ERROR, "start time not zero, need handle." );

    //
    if( subtitle.num_rects == 0 )    
        has_sub_image   =   false;
    else
    {
        has_sub_image   =   true;

        int     i, w, h;

        if( subtitle.num_rects > 1 )
            MYLOG( LOG::L_ERROR, "subtitle.num_rects = %d", subtitle.num_rects ); // 遇到再來解決,目前測試影片一次只有一張圖

        for( i = 0; i < subtitle.num_rects; i++ )
        {
            // 理論上需要做一些越界檢查等等,這邊省略了. 以後有空在說
            AVSubtitleRect  *rect   =   subtitle.rects[i];
            AVRational      ra { video_width, dec_ctx->width };

            w       =   av_rescale( rect->w, ra.num, ra.den );
            h       =   av_rescale( rect->h, ra.num, ra.den );
            sub_x   =   av_rescale( rect->x, ra.num, ra.den );
            sub_y   =   av_rescale( rect->y, ra.num, ra.den );

            int         dst_linesize[4] =   {0};
            uint8_t     *dst_data[4]    =   {nullptr};

            //
            av_image_alloc( dst_data, dst_linesize, w, h, AV_PIX_FMT_RGBA, 1 );

            SwsContext  *ctx    =   sws_getContext( rect->w, rect->h, AV_PIX_FMT_PAL8,
                                                    w,       h,       AV_PIX_FMT_RGBA,
                                                    SWS_BILINEAR, nullptr, nullptr, nullptr );

            sws_scale( ctx, rect->data, rect->linesize, 0, rect->h, dst_data, dst_linesize );

            sub_image  =    QImage( dst_data[0], w, h, QImage::Format_RGBA8888 ).copy();  // 這邊不加copy會出現破圖問題
         
            av_freep( &dst_data[0] );
            sws_freeContext(ctx);        
        }        
    }
}






/*******************************************************************************
SubDecode::is_video_in_duration()
********************************************************************************/
bool    SubDecode::is_video_in_duration( int64_t timestamp )
{
    if( has_sub_image == true && timestamp >= sub_dpts && timestamp <= sub_dpts + sub_duration )
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

    int     got_sub     =   0;
    int     ret         =   avcodec_decode_subtitle2( dec, &subtitle, &got_sub, pkt );
    
    if( ret >= 0 && got_sub > 0 )
    {
        if( got_sub > 0 )
        {
            // 代表字幕是圖片格式, 需要產生對應的字幕圖檔.
            if( subtitle.format == 0 )     
                generate_subtitle_image( subtitle );       

            avsubtitle_free( &subtitle );
            return  R_SUCCESS;
        }
        else         
            return  R_SUCCESS;  // decode success, but no subtitle.
    }
    else
    {
        MYLOG( LOG::L_ERROR, "decode subtitle fail" );
        return  R_ERROR;
    }   

#if 0
    // 用來輸出訊息的測試程式碼
    qreal pts = pkt->pts * av_q2d(subStream->time_base);
    qreal duration = pkt->duration * av_q2d(subStream->time_base);

    // https://tsduck.io/doxy/namespacets.html
    // 可以用 ts 套件做文字轉換.
    const char *text = const_int8_ptr(pkt->data);
    MYLOG( LOG::L_DEBUG, "pts = %lf, duration = %lf, text = %s", pts, duration, pkt->data );
#endif

#if 0
    // 用來輸出訊息的測試程式碼
    AVSubtitleRect **rects = subtitle.rects;
    for (int i = 0; i < subtitle.num_rects; i++) 
    {
        AVSubtitleRect rect = *rects[i];
        if (rect.type == SUBTITLE_ASS)                 
            MYLOG( LOG::L_DEBUG, "ASS %s", rect.ass)                
        else if (rect.x == SUBTITLE_TEXT)                 
            MYLOG( LOG::L_DEBUG, "TEXT %s", rect.text)
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
    MYLOG( LOG::L_INFO, "buf = %s\n", buf );

    for( auto itr : stream_map )
    {
        AVDictionaryEntry   *dic   =   av_dict_get( (const AVDictionary*)itr.second->metadata, "title", NULL, AV_DICT_MATCH_CASE );
        if( dic != nullptr )
        {
            MYLOG( LOG::L_DEBUG, "title %s", dic->value );
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
        MYLOG( LOG::L_DEBUG, "index = %d, title = %s", itr.first, dic->value );
    }

    return 0;
}





/*******************************************************************************
SubDecode::switch_subtltle()
********************************************************************************/
void    SubDecode::switch_subtltle( std::string path )
{
    set_subfile( path );

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

    if( is_graphic_subtitle() == false )
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
SubDecode::get_subtitle_image_pos()
********************************************************************************/
QPoint  SubDecode::get_subtitle_image_pos()
{
    QPoint  pos( sub_x, sub_y );
    return  pos;
}





/*******************************************************************************
SubDecode::flush_for_seek()
********************************************************************************/
void    SubDecode::flush_for_seek() 
{
    has_sub_image   =   false;
}





#ifdef FFMPEG_TEST
/*******************************************************************************
SubDecode::output_jpg_by_QT()
********************************************************************************/
int    SubDecode::output_jpg_by_QT()
{
    char    str[1000];
    sprintf( str, "%s\\%d.jpg", output_jpg_root_path.c_str(), frame_count );

    if( frame_count % 100 == 0 )
        MYLOG( LOG::L_DEBUG, "save jpg %s", str );

    sub_image.save(str);
    return  0;
}
#endif







#ifdef FFMPEG_TEST
/*******************************************************************************
VideoDecode::set_output_jpg_root()
********************************************************************************/
void    SubDecode::set_output_jpg_root( std::string _root_path )
{
    output_jpg_root_path    =   _root_path;
}
#endif






/*******************************************************************************
SubDecode::flush_all_stream()
********************************************************************************/
void    SubDecode::flush_all_stream() 
{
    int     ret     =   0;
    int     got_sub =   0;

    // for subtitle flush, data = null, size = 0. 
    // 這邊可以省略 pkt 的初始化
    AVPacket    pkt;
    pkt.data    =   nullptr;
    pkt.size    =   0;

    AVSubtitle      subtitle;

    for( auto dec : dec_map )
    {
        while(true)
        {
            ret     =   avcodec_decode_subtitle2( dec.second, &subtitle, &got_sub, &pkt );
            if( ret < 0 )
                MYLOG( LOG::L_ERROR, "flush decode subtitle fail." );

            avsubtitle_free(&subtitle);

            if( got_sub <= 0 )
                break;
            
            if( dec.first == cs_index && subtitle.format == 0 )       
                generate_subtitle_image( subtitle );
        }
    }
}







#ifdef FFMPEG_TEST
/*******************************************************************************
SubDecode::test_flush()
********************************************************************************/
int    SubDecode::flush()
{
    AVPacket    pkt;
    pkt.data    =   nullptr;
    pkt.size    =   0;

    //AVCodecContext  *dec    =   dec_map[cs_index];
    AVSubtitle      subtitle;

    int     ret, got_sub;

    for( auto dec_itr : dec_map )
    {
        while(true)
        {
            got_sub     =   0;
            ret         =   avcodec_decode_subtitle2( dec_itr.second, &subtitle, &got_sub, &pkt );
            if( ret < 0 )
                MYLOG( LOG::L_ERROR, "error." );            
            
            // if need output message, use this flag.
            //if( got_sub > 0 )
            //{}

            avsubtitle_free(&subtitle);        

            if( got_sub <= 0 )
                break;
        }
    }

    return  1;
}
#endif









/*******************************************************************************
extract_subtitle_frome_file()

ref : https://github.com/mythsaber/AudioVideo
測試用
目前有機會不能動.
********************************************************************************/
void    extract_subtitle_frome_file()
{
    static int64_t  last_pts    =   -1;    // 用來處理 flush 的 pts

    char src_file_path[1000]    =   "D:\\code\\test2.mkv";
    //char src_file_path[1000]    =   "D:\\code\\output.mkv";
    //char src_file_path[1000]    =   "J:\\abc.ass";
    char dst_file_path[1000]    =   "J:\\abccccc.ass";

    int     ret     =   0;
    int     subidx  =   0;

    AVCodec     *src_codec  =   nullptr;
    AVCodec     *dst_codec  =   nullptr;

    AVFormatContext*    src_fmtctx  =   nullptr;
    AVCodecContext*     src_dec     =   nullptr;

    AVFormatContext*    dst_fmtctx  =   nullptr;
    AVCodecContext*     dst_enc     =   nullptr;

    AVSubtitle  subtitle;

    // open input file

    src_fmtctx  =   avformat_alloc_context();
    ret         =   avformat_open_input( &src_fmtctx, src_file_path, nullptr, nullptr );
    if( ret != 0 )
        MYLOG( LOG::L_ERROR, "open fail." );

    ret     =   avformat_find_stream_info( src_fmtctx, nullptr );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "get stream fail." );

    // 第三個引數, 表示要開啟第幾個stream.
    // -1 表示自動搜尋
    // 這邊放 3 是因為影片兩個字幕軌, 我們要開啟第三個
    subidx  =   av_find_best_stream( src_fmtctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0 );
    if( subidx < 0 )
        MYLOG( LOG::L_ERROR, "find stream fail." );  

    AVStream*   src_stream    =   src_fmtctx->streams[subidx];
    src_codec   =   avcodec_find_decoder( src_stream->codecpar->codec_id );
    if( src_codec == nullptr )
        MYLOG( LOG::L_ERROR, "find decoder fail." );

    src_dec     =   avcodec_alloc_context3(src_codec);  // or avcodec_alloc_context3(nullptr);
    ret         =   avcodec_parameters_to_context( src_dec, src_stream->codecpar );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "param fail." );

    // input_decodecctx->pkt_timebase为{name=0,den=1} input_stream->time_base　{name=1,den=1000}
    // 這行刪除了會造成 timestamp 錯掉
    src_dec->pkt_timebase   =   src_stream->time_base;

    // end open input file
    // *****************************************************************************************************************************
    // open output file

    ret     =   avformat_alloc_output_context2( &dst_fmtctx, nullptr, nullptr, dst_file_path );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "open output fail." );

    // 看註解說這邊需要設置 interrupt callback, 有空研究
    ret     =   avio_open2( &dst_fmtctx->pb, dst_file_path, AVIO_FLAG_WRITE, &dst_fmtctx->interrupt_callback, nullptr );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "open fail." );

    AVStream*   dst_stream  =   avformat_new_stream( dst_fmtctx, nullptr );
    if( dst_stream == nullptr )
        MYLOG( LOG::L_ERROR, "get stream fail." );

    assert( dst_fmtctx->nb_streams == 1 );

    dst_stream->codecpar->codec_type    =   AVMEDIA_TYPE_SUBTITLE;
    dst_stream->codecpar->codec_id      =   av_guess_codec( dst_fmtctx->oformat, nullptr, dst_fmtctx->url, nullptr, dst_stream->codecpar->codec_type );
    dst_codec   =   avcodec_find_encoder( dst_stream->codecpar->codec_id );
    dst_enc     =   avcodec_alloc_context3(dst_codec);  
    if( dst_enc == nullptr )
        MYLOG( LOG::L_ERROR, "open encoder fail." );

    // 看註解說這邊沒影響, 有空測試一下
    // 不能不設置
    dst_enc->time_base      =   AVRational{ 1, 1 };
    dst_fmtctx->streams[0]->time_base   =   dst_enc->time_base;

    // end open output file.
    // ******************************************************************************************************************************
    // run main function

    ret     =   avcodec_open2( src_dec, src_codec, nullptr );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "open fail." );

    if( src_dec->subtitle_header )
    {
        dst_enc->subtitle_header        =   (uint8_t*)av_mallocz( src_dec->subtitle_header_size + 1);  // 沒查到為什麼要 + 1
        memcpy( dst_enc->subtitle_header, src_dec->subtitle_header, src_dec->subtitle_header_size );
        dst_enc->subtitle_header_size   =   src_dec->subtitle_header_size;
        dst_enc->subtitle_header[dst_enc->subtitle_header_size] = '\0';

        printf( "header = %s\n", dst_enc->subtitle_header );
    }

    ret     =   avcodec_open2( dst_enc, dst_codec, nullptr );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "open fail." );

    avcodec_parameters_from_context( dst_fmtctx->streams[0]->codecpar, dst_enc );

    // AVStream*   ist =   src_fmtctx->streams[subidx];  // src_stream
    if( dst_fmtctx->streams[0]->duration <= 0 && src_stream->duration > 0 )
        dst_fmtctx->streams[0]->duration = av_rescale_q( src_stream->duration, src_stream->time_base, dst_fmtctx->streams[0]->time_base );


    //ret     =   av_opt_set_int( dst_fmtctx->priv_data, "ignore_readorder",  0,       0 );
    //MYLOG( LOG::L_INFO, "ret = %d\n", ret );


    // 開始寫入
    auto subtitle_output_func = [ &subtitle, &dst_enc, &dst_fmtctx, &src_dec ] () -> int
    {
        static const int subtitle_out_max_size     =   1024*1024;

        // 看討論是DVD格式問題....
        //subtitle.pts                   +=   av_rescale_q( subtitle.start_display_time, src_dec->pkt_timebase, AVRational{ 1, AV_TIME_BASE } );
        //subtitle.end_display_time      -=   subtitle.start_display_time;
        //subtitle.start_display_time    =    0;
        int64_t sub_duration            =   subtitle.end_display_time - subtitle.start_display_time;

        uint8_t*    subtitle_out        =   (uint8_t*)av_mallocz(subtitle_out_max_size);
        int         subtitle_out_size   =   avcodec_encode_subtitle( dst_enc , subtitle_out, subtitle_out_max_size, &subtitle );

        AVPacket    pkt;
        av_new_packet( &pkt, 1 );

        if( subtitle_out_size == 0 )
            pkt.data    =   nullptr;
        else
            pkt.data    =   subtitle_out;

        pkt.size        =   subtitle_out_size;
        pkt.pts         =   av_rescale_q( subtitle.pts, AVRational { 1, AV_TIME_BASE }, dst_fmtctx->streams[0]->time_base );
        pkt.duration    =   av_rescale_q( sub_duration, src_dec->pkt_timebase, dst_fmtctx->streams[0]->time_base );
        pkt.dts         =   pkt.pts;

        last_pts    =   pkt.pts;    // 用來處理 flush 的 pts

        // note: flush 的處理可以參考 got_sub 或其他方式.
        /*if( pkt.pts > 0 )
            last_pts    =   pkt.pts;
        else
        {
            pkt.pts =   last_pts;
            pkt.dts =   last_pts;
            pkt.duration    =   0;
        }*/

        int ret =   0;
        if( subtitle_out_size != 0 )
            //ret =   av_write_frame( dst_fmtctx, &pkt );
            ret =   av_interleaved_write_frame( dst_fmtctx, &pkt );
        else 
            ret =   av_interleaved_write_frame( dst_fmtctx, &pkt );  // 沒找到相關說明...

        if( ret < 0 )
            MYLOG( LOG::L_ERROR, "write fail." );
        return 0;
    };

    printf( "%d %d\n", dst_fmtctx->streams[0]->time_base.num,  dst_fmtctx->streams[0]->time_base.den );

    //
    avformat_write_header( dst_fmtctx, nullptr );

    printf( "%d %d\n", dst_fmtctx->streams[0]->time_base.num,  dst_fmtctx->streams[0]->time_base.den );


    AVPacket    src_pkt;
    av_new_packet( &src_pkt, 1 );
    src_pkt.data    =   nullptr;
    src_pkt.size    =   0;

    int         got_sub     =   0;

    int count = 0;

    while( true )
    {
        ret     =   av_read_frame( src_fmtctx, &src_pkt );
        //printf("count = %d", ++count );
        if( ret < 0 )
            break;

        if( src_pkt.stream_index != subidx )
        {
            /*if( src_pkt.stream_index == 0 )
                printf("v   ");
            else if( src_pkt.stream_index == 1 )
                printf(" a  ");*/
            
            //if( src_pkt.stream_index != 2 )
              //  printf("pts = %6lld, dts = %6lld\n", src_pkt.pts, src_pkt.dts );

            av_packet_unref( &src_pkt );
            continue;
        }

        if( src_pkt.size > 0 )
        {
            //printf("  s pts = %6lld, dts = %6lld\n", src_pkt.pts, src_pkt.dts );

            memset( &subtitle, 0, sizeof(subtitle) );            
            ret     =   avcodec_decode_subtitle2( src_dec, &subtitle, &got_sub, &src_pkt ); 
            
            //printf("subtitle pts = %6lld\n", subtitle.pts );
            
            // pkt_timebase={1,1000}
            if( ret < 0 )
                MYLOG( LOG::L_ERROR, "decode fail." );

            if( got_sub > 0 )
                ret     =   subtitle_output_func();
            else
                MYLOG( LOG::L_DEBUG, "got < 0" );
            
            avsubtitle_free(&subtitle);
        }       
        else
            MYLOG( LOG::L_ERROR, "demux fail." );

        av_packet_unref(&src_pkt);
    }

    // flush    
    src_pkt.data   =   nullptr;
    src_pkt.size   =   0;
    src_pkt.pts    =   last_pts;
    src_pkt.dts    =   last_pts;
    src_pkt.duration       =   0;
    src_pkt.stream_index   =   0;
    

    if( dst_enc->subtitle_header != nullptr )
        av_free( dst_enc->subtitle_header );

    // note: 還有一些資料需要 free. 有空再來補
    av_write_trailer(dst_fmtctx);

    MYLOG( LOG::L_INFO, "finish encode subtitle." );
}





