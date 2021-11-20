#include "player.h"
#include "tool.h"

#include <thread>



extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libswscale/swscale.h>
#include <libavfilter/buffersrc.h>  // use for subtitle
#include <libavfilter/buffersink.h>


} // end extern "C"


static std::queue<AudioData> audio_queue;
static std::queue<VideoData> video_queue;



bool init_subtitle_filter( AVFilterContext * &buffersrcContext, AVFilterContext * &buffersinkContext, std::string args, std::string filterDesc)
{
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *output = avfilter_inout_alloc();
    AVFilterInOut *input = avfilter_inout_alloc();
    AVFilterGraph *filterGraph = avfilter_graph_alloc();
    //filterGraph = avfilter_graph_alloc();

    // lambda operator, 省略了傳入參數括號. 
    auto release = [&output, &input] 
    {
        avfilter_inout_free(&output);
        avfilter_inout_free(&input);
    };

    if (!output || !input || !filterGraph) 
    {
        release();
        return false;
    }

    // Crear filtro de entrada, necesita arg
    if (avfilter_graph_create_filter(&buffersrcContext, buffersrc, "in", args.c_str(), nullptr, filterGraph) < 0) 
    {
        //qDebug() << "Has Error: line =" << __LINE__;
        MYLOG( LOG::ERROR, "error" );
        release();
        return false;
    }

    if (avfilter_graph_create_filter(&buffersinkContext, buffersink, "out", nullptr, nullptr, filterGraph) < 0) 
    {
        //qDebug() << "Has Error: line =" << __LINE__;
        MYLOG( LOG::ERROR, "error" );
        release();
        return false;
    }

    output->name = av_strdup("in");
    output->next = nullptr;
    output->pad_idx = 0;
    output->filter_ctx = buffersrcContext;

    input->name = av_strdup("out");
    input->next = nullptr;
    input->pad_idx = 0;
    input->filter_ctx = buffersinkContext;

    int ret = avfilter_graph_parse_ptr(filterGraph, filterDesc.c_str(), &input, &output, nullptr);
    if (ret < 0) 
    {
        //qDebug() << "Has Error: line =" << __LINE__;
        MYLOG( LOG::DEBUG, "error" );
        release();
        return false;
    }

    char *str = avfilter_graph_dump( filterGraph, NULL );
    MYLOG( LOG::DEBUG, "options = %s", str );

    if (avfilter_graph_config(filterGraph, nullptr) < 0) 
    {
        //qDebug() << "Has Error: line =" << __LINE__;
        MYLOG( LOG::DEBUG, "error" );
        release();
        return false;
    }

    release();
    return true;
}



/*******************************************************************************
Player::init()
********************************************************************************/
int     Player::init()
{
    if( src_filename == "" )
    {
        MYLOG( LOG::ERROR, "src file not set." );
        return  ERROR;
    }

    int     ret     =   -1;
    int     vs_idx  =   -1;
    int     as_idx  =   -1;
    int     ss_idx  =   -1;

    AVFormatContext *fmt_ctx    =   nullptr;

    //
    ret     =   demuxer.open_input( src_filename );
    assert( ret == SUCCESS );

    ret     =   demuxer.init();
    assert( ret == SUCCESS );

    fmt_ctx =   demuxer.get_format_context();
    vs_idx  =   demuxer.get_video_index();
    as_idx  =   demuxer.get_audio_index();
    ss_idx  =   demuxer.get_sub_index();

    //
    ret     =   v_decoder.open_codec_context( vs_idx, fmt_ctx );
    assert( ret == SUCCESS );
    ret     =   a_decoder.open_codec_context( as_idx, fmt_ctx );
    assert( ret == SUCCESS );
    ret     =   s_decoder.open_codec_context( ss_idx, fmt_ctx );
    assert( ret == SUCCESS );
 
    //
    ret     =   v_decoder.init();
    assert( ret == SUCCESS );
    ret     =   a_decoder.init();
    assert( ret == SUCCESS );
    ret     =   s_decoder.init();
    assert( ret == SUCCESS );

    // for test
    subStream = fmt_ctx->streams[as_idx];


    //std::string filterDesc = "subtitles=filename=../../test.ass:original_size=1280x720";
    //std::string filterDesc = "subtitles=filename='\\D\\:\\\\code\\\\test2.mkv':original_size=1280x720";  // 成功的範例
    //std::string filterDesc = "subtitles=filename='\\D\\:/code/test.ass':original_size=1280x720";  // 成功的範例
    std::string filterDesc = "subtitles=filename='\\D\\:/code/test2.mkv':original_size=1920x1080:stream_index=1";  // 成功的範例


    //.arg(subtitleFilename).arg(m_width).arg(m_height);
    //    file:///D:/

    int ddd = v_decoder.get_decode_context()->sample_aspect_ratio.den;
    int nnn = v_decoder.get_decode_context()->sample_aspect_ratio.num;
    //pixel_aspect need equal ddd/nnn

    AVRational time_base = fmt_ctx->streams[vs_idx]->time_base;
    int num = time_base.num;
    int den = time_base.den;
    
    //std::string args = "video_size=1280x720:pix_fmt=0:time_base=1/24000:pixel_aspect=0/1";
    std::string args = "video_size=1920x1080:pix_fmt=64:time_base=1/1000:pixel_aspect=1/1";
                                                         //  num den                nnn ddd   
    //m_width, m_height, videoCodecContext->pix_fmt, time_base.num, time_base.den,
    //videoCodecContext->sample_aspect_ratio.num, videoCodecContext->sample_aspect_ratio.den);


    subtitleOpened = init_subtitle_filter(buffersrcContext, buffersinkContext, args, filterDesc );




    return SUCCESS;
}





/*******************************************************************************
get_audio_queue()
********************************************************************************/
std::queue<AudioData>* get_audio_queue()
{
    return  &audio_queue;
}





/*******************************************************************************
get_video_queue
********************************************************************************/
std::queue<VideoData>* get_video_queue()
{
    return &video_queue;
}




/*******************************************************************************
Player::Player()
********************************************************************************/
Player::Player()
{}





/*******************************************************************************
Player::Player()
********************************************************************************/
void    Player::set_input_file( std::string path )
{
    src_filename    =   path;
}




/*******************************************************************************
Player::is_set_input_file()
********************************************************************************/
bool    Player::is_set_input_file()
{
    return  src_filename.size() != 0;
}




/*******************************************************************************
Player::~Player()
********************************************************************************/
Player::~Player()
{}





/*******************************************************************************
Player::get_video_setting()
********************************************************************************/
VideoSetting    Player::get_video_setting()
{
    VideoSetting    vs;
    vs.width    =   demuxer.get_video_width();
    vs.height   =   demuxer.get_video_height();
    return  vs;
}





/*******************************************************************************
Player::get_audio_setting()
********************************************************************************/
AudioSetting    Player::get_audio_setting()
{
    AudioSetting    as;
    as.channel      =   demuxer.get_audio_channel();
    as.sample_rate  =   demuxer.get_audio_sample_rate();
    return  as;
}







#ifdef FFMPEG_TEST
/*******************************************************************************
Player::play()

這個函數用於 cmd debug 用

如何直接輸出 packet 的範例
fwrite( pkt->data, pkt->size, 1, fp );
printf("write video data %d bytes.\n", pkt->size);

可以取得frame,並對資訊做輸出. 實際上這部分操作被包在decoder內.
frame   =   dc->get_frame();
********************************************************************************/
void    Player::play()
{
    int         count   =   0;
    int         ret     =   0;
    AVPacket*   pkt     =   nullptr;
    Decode      *dc     =   nullptr;
    AVFrame     *frame  =   nullptr;

    // read frames from the file 
    while( true ) 
    {
        ret = demuxer.demux();
        if( ret < 0 )
        {
            printf("play end.\n");
            break;
        }
        
        pkt     =   demuxer.get_packet();
        if( pkt->stream_index == demuxer.get_video_index() )
            dc  =   dynamic_cast<Decode*>(&v_decoder);        
        else if( pkt->stream_index == demuxer.get_audio_index() )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else        
            ERRLOG("stream type not handle.");        

        //
        ret     =   dc->send_packet(pkt);
        if( ret >= 0 )
        {
            while(true)
            {
                ret     =   dc->recv_frame();
                if( ret <= 0 )
                    break;


                dc->output_frame_func();
                dc->unref_frame();
            }
        }

        demuxer.unref_packet();
    }

    v_decoder.flush();
    a_decoder.flush();

    printf("Demuxing succeeded.\n");
}
#endif





#ifdef USE_MT
/*******************************************************************************
Player::video_decode()
********************************************************************************/
void    Player::video_decode()
{
    AVPacket*   pkt     =   nullptr;
    int         ret     =   0;
    VideoData   vdata;

    // 需要加上結束判斷
    v_thr_start =   true;
    while( true ) 
    {
        while( video_pkt_queue.empty() == true )
            SLEEP_1MS;

        pkt     =   video_pkt_queue.front();
        video_pkt_queue.pop();

        //
        ret     =   v_decoder.send_packet(pkt);
        if( ret >= 0 )
        {
            while(true)
            {
                ret     =   v_decoder.recv_frame();
                if( ret <= 0 )
                    break;

                //v_decoder.output_video_frame_info();
                vdata   =   v_decoder.output_video_data();
                video_queue.push(vdata);                

                v_decoder.unref_frame();
            }
        }

        demuxer.collect_packet(pkt);
        SLEEP_1MS;
    }

    // 需要加入flush
}
#endif






#ifdef USE_MT
/*******************************************************************************
Player::audio_decode()
********************************************************************************/
void    Player::audio_decode()
{
    AVPacket*   pkt     =   nullptr;
    int         ret     =   0;
    AudioData   adata;

    // 需要加上結束判斷
    a_thr_start =   true;
    while( true ) 
    {
        while( audio_pkt_queue.empty() == true )
            SLEEP_1MS;

        pkt     =   audio_pkt_queue.front();
        audio_pkt_queue.pop();

        //
        ret     =   a_decoder.send_packet(pkt);
        if( ret >= 0 )
        {
            while(true)
            {
                ret     =   a_decoder.recv_frame();
                if( ret <= 0 )
                    break;

                //v_decoder.output_video_frame_info();
                adata   =   a_decoder.output_audio_data();
                audio_queue.push(adata);

                a_decoder.unref_frame();
            }
        }

        demuxer.collect_packet(pkt);
        SLEEP_1MS;
    }

    // 需要加上flush
}
#endif



#ifdef USE_MT
/*******************************************************************************
Player::play_QT_multi_thread()
********************************************************************************/
void    Player::play_QT_multi_thread()
{
    //int         count   =   0;
    int         ret     =   0;
    AVPacket*   pkt     =   nullptr;

    v_thr_start     =   false;
    a_thr_start     =   false;

    video_decode_thr    =   new std::thread( &Player::video_decode, this );
    audio_decode_thr    =   new std::thread( &Player::audio_decode, this );

    if( v_thr_start == false || a_thr_start == false )
        SLEEP_10MS;

    while( true ) 
    {
        while( video_queue.size() > 100 || audio_queue.size() > 200 )      // 1080p, 10bit的影像, 必須拉大buffer
        {
            SLEEP_1MS;  // 這邊不能睡太久, 不然會造成 demux 速度來不及.
            MYLOG( LOG::DEBUG, "v queue %d, a queue %d", video_queue.size(), audio_queue.size() );
        }

        auto    pkt_pair    =   demuxer.demux_multi_thread();
        ret     =   pkt_pair.first;
        pkt     =   pkt_pair.second;
        if( ret < 0 )
            break;      

        if( pkt->stream_index == demuxer.get_video_index() )
            video_pkt_queue.push(pkt);
        else if( pkt->stream_index == demuxer.get_audio_index() )
            audio_pkt_queue.push(pkt);       
        else
        {
            MYLOG( LOG::ERROR, "stream type not handle.")
        }

        SLEEP_1MS;
    }

    //
    //flush();
    MYLOG( LOG::INFO, "play finish.")

    video_decode_thr->join();
    audio_decode_thr->join();

    delete video_decode_thr; video_decode_thr = nullptr;
    delete audio_decode_thr; audio_decode_thr = nullptr;
}
#endif







/*******************************************************************************
Player::int     Player::decode_video_and_audio()
********************************************************************************/
int     Player::decode_video_and_audio( Decode *dc, AVPacket* pkt )
{
    VideoData   vdata;
    AudioData   adata;

    static int aaaa = 0;

#if 1
    if( pkt->stream_index == demuxer.get_sub_index() )
    {
#if 1
        {
            // 可以動態切換, 但要做mutex lock
            std::string filterDesc;
            if( aaaa % 2 == 0 )
                filterDesc = "subtitles=filename='\\D\\:/code/test2.mkv':original_size=1920x1080:stream_index=0";  
            else
                filterDesc = "subtitles=filename='\\D\\:/code/test2.mkv':original_size=1920x1080:stream_index=1";  

            std::string args = "video_size=1920x1080:pix_fmt=64:time_base=1/1000:pixel_aspect=1/1";

            init_subtitle_filter( buffersrcContext, buffersinkContext,  args,  filterDesc);

        }
        aaaa++;
#endif

        //encode_sub

#if 1
        // 必須decode
        int got_sub = 0;
        AVSubtitle subtitle;
        int ret = avcodec_decode_subtitle2( s_decoder.get_decode_context(), &subtitle, &got_sub, pkt );

        if( ret >= 0 && got_sub )
        {
            MYLOG( LOG::DEBUG, "decode subtitle.");
            avsubtitle_free( &subtitle );
            return  SUCCESS;
        }
#endif

#if 0
        if (ret >= 0 && got_sub) 
        {
            qreal pts = pkt->pts * av_q2d(subStream->time_base);
            qreal duration = pkt->duration * av_q2d(subStream->time_base);

            //https://tsduck.io/doxy/namespacets.html
            //const char *text = const_int8_ptr(pkt->data);
            //MYLOG( LOG::DEBUG, "pts = %lf, duration = %lf, text = %s", pts, duration, pkt->data );


            AVSubtitleRect **rects = subtitle.rects;
            for (int i = 0; i < subtitle.num_rects; i++) 
            {
                AVSubtitleRect rect = *rects[i];
                /*if (rect.type == SUBTITLE_ASS)                 
                    MYLOG( LOG::DEBUG, "ASS %s", rect.ass)                
                else if (rect.x == SUBTITLE_TEXT)                 
                    MYLOG( LOG::DEBUG, "TEXT %s", rect.text)*/
                    
            }

            if (subtitle.format == 0)
            {

                for ( int i = 0; i < subtitle.num_rects; i++) 
                {
                    AVSubtitleRect *sub_rect = subtitle.rects[i];

                    int dst_linesize[4];
                    uint8_t *dst_data[4];
                    //
                    av_image_alloc(dst_data, dst_linesize, sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA, 1);
                    SwsContext *swsContext = sws_getContext( sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
                                                             sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA,
                                                             SWS_BILINEAR, nullptr, nullptr, nullptr);
                    sws_scale(swsContext, sub_rect->data, sub_rect->linesize, 0, sub_rect->h, dst_data, dst_linesize);
                    sws_freeContext(swsContext);
                    //這裏也使用RGBA

                    sub_img = QImage(dst_data[0], sub_rect->w, sub_rect->h, QImage::Format_RGBA8888).copy();
                    av_freep(&dst_data[0]);
                }

            }


        }
#endif

        //avsubtitle_free( &subtitle );
        //return  SUCCESS;
    }
#endif



    int     ret     =   dc->send_packet(pkt);
    if( ret >= 0 )
    {
        while(true)
        {
            ret     =   dc->recv_frame();
            if( ret <= 0 )
                break;

            if( pkt->stream_index == demuxer.get_video_index() )
            {
                //v_decoder.output_video_frame_info();
                vdata   =   v_decoder.output_video_data();

                AVFrame *frame = v_decoder.get_frame();
                //AVFrame *filter_frame = av_frame_alloc();

                if (true )
                {
                    auto filter_frame = av_frame_alloc();

                    if (av_buffersrc_add_frame_flags( buffersrcContext, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                    {
                        av_frame_unref(filter_frame);
                        av_frame_free(&filter_frame);
                        break;
                        //throw std::runtime_error("Could not feed the frame into the filtergraph.");                    
                    }
                        //break;

                    int cccc = 0;

                    while (true) 
                    {

                        ret = av_buffersink_get_frame(buffersinkContext, filter_frame);

                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) 
                        {
                            av_frame_unref(filter_frame);
                            av_frame_free(&filter_frame);
                            break;
                        }
                        else if (ret < 0) 
                        {
                            av_frame_unref(filter_frame);
                            av_frame_free(&filter_frame);
                            printf("Error");
                            break;
                        }

                        if( cccc > 0 )
                            MYLOG( LOG::DEBUG, "run this loop more than once");
                        cccc++;

                        // 1. Get frame and QImage to show 
                        //QImage  img { 1280, 720, QImage::Format_RGB888 };
                        QImage  img { 1920, 1080, QImage::Format_RGB888 };


                        // 2. Convert and write into image buffer  
                        uint8_t *dst[]  =   { img.bits() };
                        int     linesizes[4];

                        SwsContext      *sws_ctx   =   v_decoder.get_sws_ctx();


                        av_image_fill_linesizes( linesizes, AV_PIX_FMT_RGB24, filter_frame->width );
                        sws_scale( sws_ctx, filter_frame->data, (const int*)filter_frame->linesize, 0, filter_frame->height, dst, linesizes );

                        //
                        //vd.index        =   frame_count;
                        //vd.frame        =   img;
                        //vd.timestamp    =   get_timestamp();
                        vdata.frame = img;
                        //dc->unref_frame();
                        av_frame_unref(filter_frame);
                        av_frame_free(&filter_frame);

                        video_queue.push(vdata);
                    }
                }

                //av_frame_unref(frame);
                dc->unref_frame();


                //video_queue.push(vdata);
                //dc->unref_frame();
            }
            else if( pkt->stream_index == demuxer.get_audio_index() )
            {
                //a_decoder.output_audio_frame_info();
                adata   =   a_decoder.output_audio_data();
                audio_queue.push(adata);
                dc->unref_frame();

            }
            else if( pkt->stream_index == demuxer.get_sub_index() )
            {
                MYLOG( LOG::DEBUG, "test" );
            }
            else
            {
                MYLOG( LOG::ERROR, "test" );
            }

            //dc->unref_frame();
        }
    }

    return  SUCCESS;
}





/*******************************************************************************
Player::play_QT()
********************************************************************************/
void    Player::play_QT()
{
    int         count   =   0;
    int         ret     =   0;
    AVPacket*   pkt     =   nullptr;
    Decode      *dc     =   nullptr;
    AVFrame     *frame  =   nullptr;


#if 0
    必須加上mutex lock, 不然會出問題
    std::thread *thr = new std::thread( [this]() -> void {

            static int aaaa = 0;

            while(true) 
            {
                std::this_thread::sleep_for( std::chrono::seconds(1) );
                aaaa++;

                // 可以動態切換
                std::string filterDesc;
                if( aaaa % 2 == 0 )
                    filterDesc = "subtitles=filename='\\D\\:/code/test2.mkv':original_size=1920x1080:stream_index=0";  
                else
                    filterDesc = "subtitles=filename='\\D\\:/code/test2.mkv':original_size=1920x1080:stream_index=1";  

                std::string args = "video_size=1920x1080:pix_fmt=64:time_base=1/1000:pixel_aspect=1/1";

                init_subtitle_filter( buffersrcContext, buffersinkContext,  args,  filterDesc);
            }
        } );
#endif


    while( true ) 
    {
        while( video_queue.size() > 50 )        
            SLEEP_10MS;

        ret     =   demuxer.demux();
        if( ret < 0 )
            break;      

        pkt     =   demuxer.get_packet();
        if( pkt->stream_index == demuxer.get_video_index() )
            dc  =   dynamic_cast<Decode*>(&v_decoder);
        else if( pkt->stream_index == demuxer.get_audio_index() )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else if( pkt->stream_index == demuxer.get_sub_index() )
            dc  =   dynamic_cast<Decode*>(&s_decoder);  
        else
        {
            MYLOG( LOG::ERROR, "stream type not handle.");
            demuxer.unref_packet();
            continue;
        }

        //
        decode_video_and_audio( dc, pkt );

        //
        demuxer.unref_packet();
    }

    //
    flush();
    MYLOG( LOG::INFO, "play finish.")
}





/*******************************************************************************
Player::flush()

flush 過程基本上同 decode, 送 nullptr 進去
也會吐出一張 frame, 需要將此 frame 資料寫入 output.

********************************************************************************/
int    Player::flush()
{
    int     ret     =   0;

    VideoData   vdata;
    AudioData   adata;
    Decode      *dc     =   nullptr;

    // flush video
    dc      =   dynamic_cast<Decode*>(&v_decoder);
    ret     =   dc->send_packet(nullptr);
    if( ret >= 0 )
    {
        while(true)
        {
            ret     =   dc->recv_frame();
            if( ret <= 0 )
                break;

            vdata   =   v_decoder.output_video_data();
            video_queue.push(vdata);           
            
            dc->unref_frame();
        }
    }

    // flush audio
    dc      =   dynamic_cast<Decode*>(&a_decoder);
    ret     =   dc->send_packet(nullptr);
    if( ret >= 0 )
    {
        while(true)
        {
            ret     =   dc->recv_frame();
            if( ret <= 0 )
                break;

            adata   =   a_decoder.output_audio_data();
            audio_queue.push(adata);

            dc->unref_frame();
        }
    }

    // need add flush sub

    return 0;
}





/*******************************************************************************
Player::end()
********************************************************************************/
int     Player::end()
{
    v_decoder.end();
    a_decoder.end();
    demuxer.end();

    return  SUCCESS;
}







