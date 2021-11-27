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



} // end extern "C"


static std::queue<AudioData> audio_queue;
static std::queue<VideoData> video_queue;


std::mutex a_mtx;
std::mutex v_mtx;

std::mutex& get_a_mtx() { return a_mtx; }
std::mutex& get_v_mtx() { return v_mtx; }


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
    AVFormatContext *fmt_ctx    =   nullptr;

    //
    ret     =   demuxer.open_input( src_filename );
    assert( ret == SUCCESS );

    ret     =   demuxer.init();
    assert( ret == SUCCESS );
    fmt_ctx =   demuxer.get_format_context();

    //
    ret     =   v_decoder.open_codec_context( fmt_ctx );
    assert( ret == SUCCESS );
    ret     =   a_decoder.open_codec_context( fmt_ctx );
    assert( ret == SUCCESS );
    ret     =   s_decoder.open_codec_context( fmt_ctx );
    assert( ret == SUCCESS );

    //
    ret     =   v_decoder.init();
    assert( ret == SUCCESS );
    ret     =   a_decoder.init();
    assert( ret == SUCCESS );

    //
    int     ss_idx  =   s_decoder.current_index();

    std::string sub_src;

    // handle subtitle
    if( ss_idx >= 0 )
    {
        demuxer.set_exist_subtitle(true);
        ret     =   s_decoder.init();
        assert( ret == SUCCESS );
        sub_src = src_filename;
    }
    else
    {
        if( sub_name.empty() == false )
        {
            demuxer.set_exist_subtitle(true);
            ret     =   s_decoder.init();
            assert( ret == SUCCESS );
            sub_src = sub_name;
        }     
    }

    if( demuxer.exist_subtitle() == true )
    {
        int             width       =   v_decoder.get_video_width();  // �o��令�q video decoderŪ���]�w
        int             height      =   v_decoder.get_video_height();
        int             v_index     =   v_decoder.current_index();
        AVPixelFormat   pix_fmt     =   v_decoder.get_pix_fmt();

        s_decoder.init_sub_image( width, height );
        s_decoder.init_sws_ctx( width, height, pix_fmt );

        // if exist subtitle, open it.
        // �o�䦳���涶�ǰ��D, �����H�K�����涶��
        // �ݭn�W�[�q�~���ɮ�Ū���r�����\��        
        std::pair<std::string,std::string>  sub_param   =   demuxer.get_subtitle_param( v_index, width, height, sub_src, pix_fmt );
        s_decoder.open_subtitle_filter( sub_param.first, sub_param.second );
    }

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
Player::set_sub_file()
********************************************************************************/
void Player::set_sub_file( std::string str )
{
    sub_name = str;
}


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
    //vs.width    =   demuxer.get_video_width();
    //vs.height   =   demuxer.get_video_height();
    vs.width    =   v_decoder.get_video_width();
    vs.height   =   v_decoder.get_video_height();
    return  vs;
}





/*******************************************************************************
Player::get_audio_setting()
********************************************************************************/
AudioSetting    Player::get_audio_setting()
{
    AudioSetting    as;

    as.channel      =   a_decoder.get_audio_channel();
    as.sample_rate  =   a_decoder.get_audio_sample_rate();

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
            MYLOG( LOG::ERROR, "stream type not handle.");        

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

    // �ݭn�[�W�����P�_
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

    // �ݭn�[�Jflush
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

    // �ݭn�[�W�����P�_
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

    // �ݭn�[�Wflush
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
        while( video_queue.size() > 100 || audio_queue.size() > 200 )      // 1080p, 10bit���v��, �����Ԥjbuffer
        {
            SLEEP_1MS;  // �o�䤣��ΤӤ[, ���M�|�y�� demux �t�רӤ���.
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
int     Player::decode( Decode *dc, AVPacket* pkt )
{
    int     ret     =   0;

    VideoData   vdata;
    AudioData   adata;
    AVFrame     *video_frame    =   nullptr;

    // ������ subtitle �i��decode, ���M filter �|�X��
    if( s_decoder.is_index( pkt->stream_index ) == true )
    {
        ret     =   s_decoder.decode_subtitle(pkt);
        return  ret;
    }

    // handle video stream with subtitle
    if( demuxer.exist_subtitle() == true && v_decoder.is_index( pkt->stream_index ) == true )
    {
        ret     =   v_decoder.send_packet(pkt);
        if( ret >= 0 )
        {
            while(true)
            {
                ret     =   dc->recv_frame(pkt->stream_index);
                if( ret <= 0 )
                    break;

                video_frame     =   v_decoder.get_frame();
                ret     =   s_decoder.send_video_frame( video_frame );

                while(true)
                {
                    ret     =   s_decoder.render_subtitle();
                    if( ret <= 0 )
                        break;

                    vdata.frame         =   s_decoder.get_subtitle_image();
                    vdata.index         =   v_decoder.get_frame_count();
                    vdata.timestamp     =   v_decoder.get_timestamp();

                    v_mtx.lock();
                    video_queue.push(vdata);
                    v_mtx.unlock();

                    s_decoder.unref_frame();
                }

                v_decoder.unref_frame();
            }
        }

        return  1;
    }


    //��ڹB�檺�ɭԹJ��@���H��crash. (�O����X��)
    //�Ѧҭ쥻��code�A��@�U�y�{

    //
    ret     =   dc->send_packet(pkt);
    if( ret >= 0 )
    {
        while(true)
        {
            ret     =   dc->recv_frame(pkt->stream_index);
            if( ret <= 0 )
                break;

            if( pkt->stream_index == v_decoder.current_index() )
            {
                vdata   =   v_decoder.output_video_data();
                v_mtx.lock();
                video_queue.push(vdata);
                v_mtx.unlock();
            }
            else if( pkt->stream_index == a_decoder.current_index() )
            {
                //a_decoder.output_audio_frame_info();
                adata   =   a_decoder.output_audio_data();
                a_mtx.lock();
                audio_queue.push(adata);
                a_mtx.unlock();
            }     

            dc->unref_frame();
        }
    }


    return  SUCCESS;
}





/*******************************************************************************
Player::play_QT()

�n�Ҽ{�Y�Ǽv��, ���y�}�l���I����T�}�l���I���P, �n�w��o�I���վ�
�ݭnŪ�� pts ���P�B.

********************************************************************************/
void    Player::play_QT()
{
    int         count   =   0;
    int         ret     =   0;
    AVPacket*   pkt     =   nullptr;
    Decode      *dc     =   nullptr;
    AVFrame     *frame  =   nullptr;

    bool first_play_v_and_a = false;

    //
    while( true ) 
    {
        // ���J��v����,�ѤF�j�qvideo��~�Ѷ}audio, ����n�令����@�ӭ��y�Ѷ}��~�}�lsleep
        if( first_play_v_and_a == true )
        {
            while( video_queue.size() > 300 )  // value �Ӥp���J��@���� audio_queue is empty��warn.
                SLEEP_10MS;
        }

        if( first_play_v_and_a == false && video_queue.size() > 10 && audio_queue.size() > 10 )
            first_play_v_and_a = true;

        //MYLOG( LOG::DEBUG, "v = %d, a = %d", video_queue.size(), audio_queue.size() );

        ret     =   demuxer.demux();
        if( ret < 0 )
            break;      

        pkt     =   demuxer.get_packet();
        if( v_decoder.is_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&v_decoder);
        else if( a_decoder.is_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else if( s_decoder.is_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&s_decoder);  
        else
        {
            MYLOG( LOG::ERROR, "stream type not handle.");
            demuxer.unref_packet();
            continue;
        }

        //
        decode( dc, pkt );       
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
    AVFrame     *video_frame    =   nullptr;

    // flush subtitle
    //s_decoder.decode_subtitle(nullptr);

    // flush video
    dc      =   dynamic_cast<Decode*>(&v_decoder);
    ret     =   dc->send_packet(nullptr);
    if( ret >= 0 )
    {
        while(true)
        {
            ret     =   dc->recv_frame(-1);
            if( ret <= 0 )
                break;

            // flush ���q���ӷQ�B�z subtitle, ���|�����~, �٨S���ѨM�����k
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
            ret     =   dc->recv_frame(-1);
            if( ret <= 0 )
                break;

            adata   =   a_decoder.output_audio_data();

            a_mtx.lock();
            audio_queue.push(adata);
            a_mtx.unlock();

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
    s_decoder.end();
    demuxer.end();

    sub_name = "";

    return  SUCCESS;
}







