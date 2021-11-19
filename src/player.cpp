#include "player.h"
#include "tool.h"

#include <thread>


extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

} // end extern "C"


static std::queue<AudioData> audio_queue;
static std::queue<VideoData> video_queue;






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

    ret     =   v_decoder.open_codec_context( vs_idx, fmt_ctx );
    assert( ret == SUCCESS );
    ret     =   a_decoder.open_codec_context( as_idx, fmt_ctx );
    assert( ret == SUCCESS );
    ret     =   s_decoder.open_codec_context( ss_idx, fmt_ctx );
    assert( ret == SUCCESS );

    ret     =   v_decoder.init();
    assert( ret == SUCCESS );
    ret     =   a_decoder.init();
    assert( ret == SUCCESS );
    ret     =   s_decoder.init();
    assert( ret == SUCCESS );


    return SUCCESS;
}




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
Player::play_QT()
********************************************************************************/
void    Player::play_QT()
{
    int         count   =   0;
    int         ret     =   0;
    AVPacket*   pkt     =   nullptr;
    Decode      *dc     =   nullptr;
    AVFrame     *frame  =   nullptr;
    VideoData   vdata;
    AudioData   adata;

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
        {
            dc  =   dynamic_cast<Decode*>(&s_decoder);

            int gotSubtitle =0;
            AVSubtitle subtitle;
            int result = avcodec_decode_subtitle2( s_decoder.get_decode_context(), &subtitle, &gotSubtitle, pkt );
            uint64_t bufferSize = 1024 * 1024 ;
            uint8_t *buffer = new uint8_t[bufferSize];
            memset(buffer,0, bufferSize);


            if (result >= 0 && gotSubtitle) 
            {
                AVSubtitleRect **rects = subtitle.rects;
                for (int i = 0; i < subtitle.num_rects; i++) 
                {
                    AVSubtitleRect rect = *rects[i];
                    if (rect.type == SUBTITLE_ASS) 
                    {
                        printf("ASS %s", rect.ass);
                    } 
                    else if (rect.x == SUBTITLE_TEXT) 
                    {
                        printf("TEXT %s", rect.text);
                    }
                }
                // it just writes some big file (similar to videofile size)
                //out.write((char*)pkt.data, pkt.size);
            }

            /*if( result>= 0 )
            {
                result = avcodec_encode_subtitle( outStream->codec, buffer, bufferSize, &subtitle );
                cerr << "Encode subtitle result: " << result << endl;
            }
            cerr << "Encoded subtitle: " << buffer << endl;
            delete [] buffer;*/

        }
        else
        {
            MYLOG( LOG::WARN, "stream type not handle.");
            demuxer.unref_packet();
            continue;
        }

        //
        ret     =   dc->send_packet(pkt);
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
                    video_queue.push(vdata);
                }
                else if( pkt->stream_index == demuxer.get_audio_index() )
                {
                    //a_decoder.output_audio_frame_info();
                    adata   =   a_decoder.output_audio_data();
                    audio_queue.push(adata);
                }
                else if( pkt->stream_index == demuxer.get_sub_index() )
                {
                    MYLOG( LOG::DEBUG, "test" );
                }
                else
                    MYLOG( LOG::ERROR, "stream type not handle.")

                dc->unref_frame();
            }
        }

        demuxer.unref_packet();
    }

    //
    flush();
    MYLOG( LOG::INFO, "play finish.")
}


#if 0
static void decode(struct sd *sd, struct demux_packet *packet)
{
    struct sd_lavc_priv *priv = sd->priv;
    AVCodecContext *avctx = priv->avctx;
    double ts = av_q2d(av_inv_q(avctx->time_base));
    AVSubtitle sub = {0};
    AVPacket pkt;
    int ret, got_sub;

    mp_set_av_packet(&pkt, packet);
    pkt.pts = packet->pts == MP_NOPTS_VALUE ? AV_NOPTS_VALUE : packet->pts * ts;
    pkt.duration = packet->duration * ts;

    ret = avcodec_decode_subtitle2(avctx, &sub, &got_sub, &pkt);
    if (ret < 0) {
        mp_msg(MSGT_OSD, MSGL_ERR, "Error decoding subtitle\n");
    } else if (got_sub) {
        for (int i = 0; i < sub.num_rects; i++) {
            char *ass_line = sub.rects[i]->ass;
            if (!ass_line)
                break;
            // This might contain embedded timestamps, using the "old" ffmpeg
            // ASS packet format, in which case pts/duration might be ignored
            // at a later point.
            sd_conv_add_packet(sd, ass_line, strlen(ass_line),
                packet->pts, packet->duration);
        }
    }

    avsubtitle_free(&sub);
}
#endif


#if 0
while( av_read_frame( context,&pkt ) >=0 )
{
    if(pkt.stream_index!= stream.index)
        continue;
    int gotSubtitle =0;
    AVSubtitle subtitle;
    result = avcodec_decode_subtitle2( avstream->codec,&subtitle, &gotSubtitle, &pkt );
    uint64_t bufferSize = 1024 * 1024 ;
    uint8_t *buffer= new uint8_t[bufferSize];
    memset(buffer,0, bufferSize);
    if( result>= 0 )
    {
        result = avcodec_encode_subtitle( outStream->codec, buffer, bufferSize, &subtitle );
        cerr << "Encode subtitle result: " << result << endl;
    }
    cerr << "Encoded subtitle: " << buffer << endl;
    delete [] buffer;
}
#endif


#if 0
int test()
{


    void saveSubtitle( AVFormatContext*context, Stream stream )
    {
        stringstream outfile;
        outfile << "/tmp/subtitle_"<< stream.index << ".srt";

        string filename = outfile.str();

        AVStream *avstream = context->streams[stream.index];
        AVCodec *codec = avcodec_find_decoder( avstream->codec->codec_id);
        
        int result = avcodec_open2( avstream->codec, codec,NULL );
        checkResult( result == 0, "Error opening codec");
        cerr << "found codec: " << codec <<", open result= " << result << endl;

        AVOutputFormat *outFormat = av_guess_format( NULL, filename.c_str(),NULL );
        checkResult( outFormat != NULL, "Error finding format" );
        cerr << "Found output format: " << outFormat->name<< " (" << outFormat->long_name<< ")" << endl;

        AVFormatContext *outFormatContext;
        avformat_alloc_output_context2( &outFormatContext, NULL, NULL, filename.c_str());

        AVCodec *encoder = avcodec_find_encoder( outFormat->subtitle_codec);
        checkResult( encoder != NULL, "Error finding encoder" );
        cerr << "Found encoder: " << encoder->name<< endl;

        AVStream *outStream = avformat_new_stream( outFormatContext, encoder );
        checkResult( outStream != NULL, "Error allocating out stream" );

        AVCodecContext *c = outStream->codec;
        result = avcodec_get_context_defaults3( c, encoder);
        checkResult( result == 0, "error on get context default");
        cerr << "outstream codec: " << outStream->codec<< endl;
        cerr << "Opened stream " << outStream->id<< ", codec=" << outStream->codec->codec_id<< endl;

        result = avio_open(&outFormatContext->pb, filename.c_str(), AVIO_FLAG_WRITE);
        checkResult( result == 0, "Error opening out file");
        cerr << "out file opened correctly" << endl;

        result = avformat_write_header( outFormatContext,NULL );
        checkResult(result==0,"Error writing header");
        cerr << "header wrote correctly" << endl;
        result = 0;

        AVPacket pkt;
        av_init_packet( &pkt);
        pkt.data = NULL;
        pkt.size = 0;
        cerr << "srt codec id: " << AV_CODEC_ID_SUBRIP << endl;
        while( av_read_frame( context,&pkt ) >=0 )
        {
            if(pkt.stream_index!= stream.index)
                continue;
            int gotSubtitle =0;
            AVSubtitle subtitle;
            result = avcodec_decode_subtitle2( avstream->codec,&subtitle, &gotSubtitle, &pkt );
            uint64_t bufferSize = 1024 * 1024 ;
            uint8_t *buffer= new uint8_t[bufferSize];
            memset(buffer,0, bufferSize);
            if( result>= 0 )
            {
                result = avcodec_encode_subtitle( outStream->codec, buffer, bufferSize, &subtitle );
                cerr << "Encode subtitle result: " << result << endl;
            }
            cerr << "Encoded subtitle: " << buffer << endl;
            delete [] buffer;
        }
    }
}
#endif


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
