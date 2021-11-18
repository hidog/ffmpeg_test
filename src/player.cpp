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

    AVFormatContext *fmt_ctx    =   nullptr;

    //
    ret     =   demuxer.open_input( src_filename );
    assert( ret == SUCCESS );

    ret     =   demuxer.init();
    assert( ret == SUCCESS );

    fmt_ctx =   demuxer.get_format_context();
    vs_idx  =   demuxer.get_video_index();
    as_idx  =   demuxer.get_audio_index();

    ret     =   v_decoder.open_codec_context( vs_idx, fmt_ctx );
    assert( ret == SUCCESS );
    ret     =   a_decoder.open_codec_context( as_idx, fmt_ctx );
    assert( ret == SUCCESS );

    ret     =   v_decoder.init();
    assert( ret == SUCCESS );
    ret     =   a_decoder.init();
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

�o�Ө�ƥΩ� cmd debug ��

�p�󪽱���X packet ���d��
fwrite( pkt->data, pkt->size, 1, fp );
printf("write video data %d bytes.\n", pkt->size);

�i�H���oframe,�ù��T����X. ��ڤW�o�����ާ@�Q�]�bdecoder��.
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



void    Player::video_decode()
{
    AVPacket*   pkt     =   nullptr;
    int         ret     =   0;
    VideoData   vdata;

    //
    while( true ) 
    {
        while( video_pkt_queue.empty() == true )
            SLEEP_10MS;

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

        av_packet_unref(pkt);
        av_packet_free( &pkt );
    }
}


void    Player::audio_decode()
{
    AVPacket*   pkt     =   nullptr;
    int         ret     =   0;
    AudioData   adata;

    //
    while( true ) 
    {
        while( audio_pkt_queue.empty() == true )
            SLEEP_10MS;

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

        av_packet_unref(pkt);
        av_packet_free( &pkt );
    }
}



/*******************************************************************************
Player::play_QT_multi_thread()
********************************************************************************/
void    Player::play_QT_multi_thread()
{
    int         count   =   0;
    int         ret     =   0;
    AVPacket*   pkt     =   nullptr;
    Decode      *dc     =   nullptr;
    //AVFrame     *frame  =   nullptr;
    VideoData   vdata;
    AudioData   adata;

    video_decode_thr    =   new std::thread( &Player::video_decode, this );
    audio_decode_thr    =   new std::thread( &Player::audio_decode, this );



    while( true ) 
    {
        while( video_queue.size() > 40 || audio_queue.size() > 40 )        
            SLEEP_10MS;

        ret     =   demuxer.demux();
        if( ret < 0 )
            break;      

        pkt     =   demuxer.get_packet();

        //AVPacket* npkt =  av_packet_alloc();
        //av_copy_packet( npkt, pkt );
        //av_packet_ref(npkt, pkt);

//#define AV_INPUT_BUFFER_PADDING_SIZE   64



//#undef AV_INPUT_BUFFER_PADDING_SIZE

        if( pkt->stream_index == demuxer.get_video_index() )
        {
            AVPacket *npkt = av_packet_clone(pkt);
            //AVPacket *npkt  =   av_packet_alloc();
            //npkt->data = reinterpret_cast<uint8_t*>(new uint64_t[(pkt->size + AV_INPUT_BUFFER_PADDING_SIZE)/sizeof(uint64_t) + 1]);
            //memcpy(npkt->data, pkt->data, pkt->size);
            video_pkt_queue.push(npkt);
            //dc  =   dynamic_cast<Decode*>(&v_decoder);
        }
        else if( pkt->stream_index == demuxer.get_audio_index() )
        {
            AVPacket *npkt = av_packet_clone(pkt);
            //AVPacket *npkt  =   av_packet_alloc();
            //npkt->data = reinterpret_cast<uint8_t*>(new uint64_t[(pkt->size + AV_INPUT_BUFFER_PADDING_SIZE)/sizeof(uint64_t) + 1]);
            //memcpy(npkt->data, pkt->data, pkt->size);
            //dc  =   dynamic_cast<Decode*>(&a_decoder);
            audio_pkt_queue.push(npkt);
        }
        else
        {
            //demuxer.unref_packet();
            continue;
            //MYLOG( LOG::ERROR, "stream type not handle.")
        }

        //
        //demuxer.unref_packet();

#if 0
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
                else
                    MYLOG( LOG::ERROR, "stream type not handle.")

                    dc->unref_frame();
            }
        }
#endif

    }

    //
    //flush();
    MYLOG( LOG::INFO, "play finish.")

    video_decode_thr->join();
    audio_decode_thr->join();

    delete video_decode_thr; video_decode_thr = nullptr;
    delete audio_decode_thr; audio_decode_thr = nullptr;
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
    //AVFrame     *frame  =   nullptr;
    VideoData   vdata;
    AudioData   adata;

    while( true ) 
    {
        while( video_queue.size() > 40 )        
            SLEEP_10MS;

        ret     =   demuxer.demux();
        if( ret < 0 )
            break;      

        pkt     =   demuxer.get_packet();
        if( pkt->stream_index == demuxer.get_video_index() )
            dc  =   dynamic_cast<Decode*>(&v_decoder);
        else if( pkt->stream_index == demuxer.get_audio_index() )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else
        {
            demuxer.unref_packet();
            continue;
            //MYLOG( LOG::ERROR, "stream type not handle.")
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



/*******************************************************************************
Player::flush()

flush �L�{�򥻤W�P decode, �e nullptr �i�h
�]�|�R�X�@�i frame, �ݭn�N�� frame ��Ƽg�J output.

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
