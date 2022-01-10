#include "player.h"
#include "tool.h"
#include "demux_io.h"

#include <thread>
#include <QPainter>

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

} // end extern "C"


static std::queue<AudioData>    audio_queue;
static std::queue<VideoData>    video_queue;

static std::mutex  a_mtx;
static std::mutex  v_mtx;

static bool ui_v_seek_lock  =   false;  // �bseek�ƥ󪺮ɭ�,�t�d��UI��video worker, audio worker���P�B
static bool ui_a_seek_lock  =   false;







/*******************************************************************************
get_v_seek_lock()
********************************************************************************/
bool&   get_v_seek_lock() 
{ 
    return ui_v_seek_lock; 
}





/*******************************************************************************
get_a_seek_lock()
********************************************************************************/
bool&   get_a_seek_lock() 
{
    return ui_a_seek_lock; 
}






/*******************************************************************************
get_a_mtx()
********************************************************************************/
std::mutex& get_a_mtx() 
{ 
    return a_mtx; 
}





/*******************************************************************************
get_v_mtx()
********************************************************************************/
std::mutex& get_v_mtx() 
{ 
    return v_mtx; 
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
Player::init_demuxer()
********************************************************************************/
int     Player::init_demuxer()
{
    if( demuxer != nullptr )
        MYLOG( LOG::ERROR, "demuxer not null." );
    if( setting.io_type == IO_Type::DEFAULT )
    {
        demuxer     =   new Demux{};
        if( setting.filename.empty() == true )
        {
            MYLOG( LOG::ERROR, "src file is empty." );
            return  ERROR;   
        }
        demuxer->set_input_file( setting.filename );
    }
    else
    {
        demuxer     =   new DemuxIO{};
    }

    return  SUCCESS;
}







/*******************************************************************************
Player::init()
********************************************************************************/
int     Player::init()
{
    int     ret{0};

    // init demux
    ret     =   init_demuxer();
    if( ret != SUCCESS )
    {
        MYLOG( LOG::ERROR, "init demux fail." );
        return  ret;
    }

    //
    stop_flag   =   false;
    seek_flag   =   false;

    AVFormatContext *fmt_ctx    =   nullptr;

    //
    ret     =   demuxer->open_input();
    ret     =   demuxer->init();
    fmt_ctx =   demuxer->get_format_context();

    //
    ret     =   v_decoder.open_codec_context( fmt_ctx );
    ret     =   a_decoder.open_codec_context( fmt_ctx );
    ret     =   s_decoder.open_codec_context( fmt_ctx );

    //
    ret     =   v_decoder.init();
    ret     =   a_decoder.init();    

    // handle subtitle
    // ���J��v���|�b�o��d�ܤ[, �γ\�i�H�Ҽ{��multi-thread���覡���B�z, �H��A��...
    //init_subtitle(fmt_ctx);

    return SUCCESS;
}




#ifdef FFMPEG_TEST
/*******************************************************************************
Player::set_output_openCV_jpg_root()
********************************************************************************/
void    Player::set_output_jpg_root( std::string _root_path )
{
    v_decoder.set_output_jpg_root(_root_path);
}
#endif





#ifdef FFMPEG_TEST
/*******************************************************************************
Player::set_output_audio_pcm_path()
********************************************************************************/
void    Player::set_output_audio_pcm_path( std::string _path )
{
    a_decoder.set_output_audio_pcm_path(_path);
}
#endif







/*******************************************************************************
Player::init_subtitle()
********************************************************************************/
void    Player::init_subtitle( AVFormatContext *fmt_ctx )
{
    int             ret;
    bool            exist_subtitle  =   false;
    SubData         sd;
    std::string     sub_src;

    std::pair<std::string,std::string>  sub_param;

    if( s_decoder.exist_stream() == true )
    {
        exist_subtitle  =   true;
        s_decoder.set_subfile( setting.filename );
        s_decoder.set_sub_src_type( SubSourceType::EMBEDDED );
    }
    else
    {
        if( setting.subname.empty() == false )
        {
            exist_subtitle  =   true;
            s_decoder.set_subfile( setting.subname );
            s_decoder.set_sub_src_type( SubSourceType::FROM_FILE );
        }
        else
        {
            exist_subtitle  =   false;
            s_decoder.set_sub_src_type( SubSourceType::NONE );
        }
    }

    //
    if( exist_subtitle == true )
    {
        ret     =   s_decoder.init();

        sd.width        =   v_decoder.get_video_width();
        sd.height       =   v_decoder.get_video_height();
        sd.pix_fmt      =   v_decoder.get_pix_fmt();
        sd.video_index  =   v_decoder.current_index();
        sd.sub_index    =   0;

        if( s_decoder.is_graphic_subtitle() == true )
            s_decoder.init_graphic_subtitle(sd);        
        else
        {
            sub_src     =   s_decoder.get_subfile();

            s_decoder.init_sws_ctx( sd );

            // if exist subtitle, open it.
            // �o�䦳���涶�ǰ��D, �����H�K�����涶��      
            sub_param   =   s_decoder.get_subtitle_param( fmt_ctx, sub_src, sd );
            s_decoder.open_subtitle_filter( sub_param.first, sub_param.second );
            s_decoder.set_filter_args( sub_param.first );
        }       
    }
}




/*******************************************************************************
Player::Player()
********************************************************************************/
Player::Player()
{}






/*******************************************************************************
Player::set_sub_file()
********************************************************************************/
/*void Player::set_sub_file( std::string str )
{
    subname    =   str;
}*/





/*******************************************************************************
Player::Player()
********************************************************************************/
void    Player::set( DecodeSetting _setting )
{
    setting     =   _setting;
}




/*******************************************************************************
Player::is_set_input_file()
********************************************************************************/
bool    Player::is_set_input_file()
{
    return  setting.filename.empty();
}






/*******************************************************************************
Player::~Player()
********************************************************************************/
Player::~Player()
{}





/*******************************************************************************
Player::get_video_setting()
********************************************************************************/
VideoDecodeSetting    Player::get_video_setting()
{
    VideoDecodeSetting    vs;
    vs.width    =   v_decoder.get_video_width();
    vs.height   =   v_decoder.get_video_height();
    return  vs;
}





/*******************************************************************************
Player::get_audio_setting()
********************************************************************************/
AudioDecodeSetting    Player::get_audio_setting()
{
    AudioDecodeSetting    as;
    as.channel      =   a_decoder.get_audio_channel();
    as.sample_rate  =   a_decoder.get_audio_sample_rate();
    return  as;
}




/*******************************************************************************
Player::get_duration_time
********************************************************************************/
int64_t     Player::get_duration_time()
{
    if( demuxer == nullptr )  
        MYLOG( LOG::ERROR, "demuxer is null." );
    return  demuxer->get_duration_time();
}



#ifdef FFMPEG_TEST
/*******************************************************************************
Player::play_decode_video_subtitle()
********************************************************************************/
void    Player::play_decode_video_subtitle( AVPacket* pkt )
{
    AVFrame     *frame  =   nullptr;
    int         ret     =   v_decoder.send_packet(pkt);

    if( ret >= 0 )
    {
        while(true)
        {
            ret     =   v_decoder.recv_frame(pkt->stream_index);
            if( ret <= 0 )
                break;

            frame   =   v_decoder.get_frame();
            ret     =   s_decoder.send_video_frame( frame );

            while(true) // note: �ثe���յ��G���O�@���@�i,���T�w�O�_�s�b�@����i��case
            {
                ret     =   s_decoder.render_subtitle();
                if( ret <= 0 )
                    break;

                s_decoder.output_frame_func();
                s_decoder.unref_frame();
            }

            v_decoder.unref_frame();
        }
    }
}
#endif




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
    if( demuxer == nullptr )
        MYLOG( LOG::ERROR, "demuxer is null." );

    int         count   =   0;
    int         ret     =   0;
    AVPacket*   pkt     =   nullptr;
    Decode      *dc     =   nullptr;
    AVFrame     *frame  =   nullptr;

    // read frames from the file 
    while( true ) 
    {
        ret     =   demuxer->demux();
        if( ret < 0 )
        {
            printf("play end.\n");
            break;
        }
        
        pkt     =   demuxer->get_packet();
        if( v_decoder.find_index(pkt->stream_index) )
            dc  =   dynamic_cast<Decode*>(&v_decoder);        
        else if( a_decoder.find_index(pkt->stream_index) )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else if( s_decoder.find_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&s_decoder);
        else        
        {
            MYLOG( LOG::ERROR, "stream type not handle.");
            demuxer->unref_packet();
            continue;
        }

        //
        if( dc == &s_decoder )
            s_decoder.decode_subtitle( pkt );
#ifdef RENDER_SUBTITLE
        // �|���B�z graphic subtitle. �H�ᦳ�ݭn�A�s�W.
        else if( s_decoder.exist_stream() == true && dc == &v_decoder )
            play_decode_video_subtitle( pkt );
#endif
        else
        {
            //
            ret     =   dc->send_packet(pkt);
            if( ret >= 0 )
            {
                while(true)
                {
                    ret     =   dc->recv_frame(pkt->stream_index);
                    if( ret <= 0 )
                        break;

                    if( dc->output_frame_func != nullptr )
                        dc->output_frame_func();
                    dc->unref_frame();
                }
            }
        }
        demuxer->unref_packet();
    }

    // flush
    if( s_decoder.exist_stream() == true )
        s_decoder.flush();
#ifdef RENDER_SUBTITLE
    ret     =   v_decoder.send_packet(nullptr);
    if( ret >= 0 )
    {       
        while(true)
        {
            ret     =   v_decoder.recv_frame(-1);
            if( ret <= 0 )
                break;

            frame   =   v_decoder.get_frame();
            ret     =   s_decoder.send_video_frame( frame );

            while(true)
            {
                // �o��@�˨S�B�z subtitle �� image �� case.
                ret     =   s_decoder.render_subtitle();
                if( ret <= 0 )
                    break;

                s_decoder.output_frame_func();
                s_decoder.unref_frame();
            }

            v_decoder.unref_frame();  
        }
    }
#else
    v_decoder.flush();
#endif
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
                ret     =   v_decoder.recv_frame(pkt->stream_index);
                if( ret <= 0 )
                    break;

                //v_decoder.output_video_frame_info();
                vdata   =   v_decoder.output_video_data();
                video_queue.push(vdata);                

                v_decoder.unref_frame();
            }
        }

        demuxer.collect_packet(pkt);
        //SLEEP_1MS;
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
                ret     =   a_decoder.recv_frame(pkt->stream_index);
                if( ret <= 0 )
                    break;

                //v_decoder.output_video_frame_info();
                adata   =   a_decoder.output_audio_data();
                audio_queue.push(adata);

                a_decoder.unref_frame();
            }
        }

        demuxer.collect_packet(pkt);
        //SLEEP_1MS;
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

    while( v_thr_start == false || a_thr_start == false )
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
        if( pkt == nullptr )
        {
            MYLOG( LOG::ERROR, "pkt is nullptr" );
            break;
        }

        if( v_decoder.find_index( pkt->stream_index ) == true )
            video_pkt_queue.push(pkt);
        else if( a_decoder.find_index( pkt->stream_index ) == true )
            audio_pkt_queue.push(pkt);       
        else
        {
            MYLOG( LOG::DEBUG, "stream type not handle.")
            demuxer.collect_packet(pkt);
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
Player::demux_need_wait()

���F�קK queue �Q���z,�q�L�j���ɭԷ|���U�ӵ� UI �ݮ���.
�n�Ҽ{�u��video or audio stream.

********************************************************************************/
bool    Player::demux_need_wait()
{
    if( seek_flag == true )
        return  false;
    else if( v_decoder.exist_stream() == true && a_decoder.exist_stream() == true )
    {
        if( video_queue.size() >= MAX_QUEUE_SIZE && audio_queue.size() >= MAX_QUEUE_SIZE )
            return  true;
        else
            return  false;
    }
    else if( v_decoder.exist_stream() == true && video_queue.size() >= MAX_QUEUE_SIZE )    
        return  true;  
    else if( a_decoder.exist_stream() == true && audio_queue.size() >= MAX_QUEUE_SIZE )
        return  true;
    else
        return  false;
}




/*******************************************************************************
Player::is_embedded_subtitle()
********************************************************************************/
bool    Player::is_embedded_subtitle()
{
    return  s_decoder.get_sub_src_type() == SubSourceType::EMBEDDED;
}






/*******************************************************************************
Player::is_file_subtitle()
********************************************************************************/
bool    Player::is_file_subtitle()
{
    return  s_decoder.get_sub_src_type() == SubSourceType::FROM_FILE;
}





/*******************************************************************************
Player::is_embedded_subtitle()
********************************************************************************/
std::vector<std::string>    Player::get_embedded_subtitle_list()
{
    return  s_decoder.get_embedded_subtitle_list();
}





/*******************************************************************************
Player::stop()
********************************************************************************/
void    Player::stop()
{
    stop_flag   =   true;
}






/*******************************************************************************
Player::handle_seek()

�ݰQ��, avformat_seek_file �� av_seek_frame �n
���Ӹ`�ݭn��s.
********************************************************************************/
void    Player::handle_seek()
{
    int         ret;
    AudioData   adata;

    // clear queue.
    v_mtx.lock();
    while( video_queue.empty() == false )
        video_queue.pop();
    v_mtx.unlock();

    a_mtx.lock();
    while( audio_queue.empty() == false )
    {
        adata   =   audio_queue.front();
        delete [] adata.pcm;
        audio_queue.pop();
    }
    a_mtx.unlock();

    v_decoder.flush_for_seek();
    a_decoder.flush_for_seek();
    s_decoder.flush_for_seek();

    // run seek.
    AVFormatContext*    fmt_ctx     =   demuxer->get_format_context();
    avformat_flush( fmt_ctx );  // �ݰ_�ӬO�������~�ݭn���o�Ӱʧ@,�����T�w

    int64_t     so  =   seek_old,
                sv  =   seek_value; // �קKoverflow

    // ���o�˳]�m����, seek�j�d��|�X��.  �Ҧp�W�L�@�p�ɪ��v��, ���� seek �W�L�b�p�ɫ�
    int64_t     min     =   so < sv ? so * AV_TIME_BASE + 2 : INT64_MIN,
                max     =   so > sv ? so * AV_TIME_BASE - 2 : INT64_MAX,
                ts      =   sv * AV_TIME_BASE;       

    ret     =   avformat_seek_file( fmt_ctx, -1, min, ts, max, 0 );  //AVSEEK_FLAG_ANY
    if( ret < 0 )
        MYLOG( LOG::DEBUG, "seek fail." );

}







/*******************************************************************************
Player::play_QT()
********************************************************************************/
void    Player::play_QT()
{
    int         ret     =   0;
    AVPacket    *pkt    =   nullptr;
    Decode      *dc     =   nullptr;

    //
    while( stop_flag == false ) 
    {
        // NOTE: seek�ƥ�Ĳ�o���ɭ�, queue ��Ʒ|�ɼW.
        while( demux_need_wait() == true )
        {
            //MYLOG( LOG::DEBUG, "v size = %d, a size = %d", video_queue.size(), audio_queue.size() );
            if( stop_flag == true )
                break;
            SLEEP_1MS;
        }

        //
        if( seek_flag == true )     
        {
            while( ui_v_seek_lock == false || ui_a_seek_lock == false )
                SLEEP_10MS;
            seek_flag   =   false;
            handle_seek();
            ui_v_seek_lock = false;
            ui_a_seek_lock = false;
        }

        //
        ret     =   demuxer->demux();
        if( ret < 0 )
        {
            MYLOG( LOG::INFO, "demux finish.");
            break;
        }

        //
        pkt     =   demuxer->get_packet();
        if( v_decoder.find_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&v_decoder);
        else if( a_decoder.find_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else if( s_decoder.find_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&s_decoder);  
        else
        {
            MYLOG( LOG::ERROR, "stream type not handle.");
            demuxer->unref_packet();
            continue;
        }

        //
        decode( dc, pkt );       
        demuxer->unref_packet();
    }

    //
    flush();
    MYLOG( LOG::INFO, "play finish.")
}





/*******************************************************************************
Player::decode
********************************************************************************/
int     Player::decode( Decode *dc, AVPacket* pkt )
{
    // switch subtitle track
    // �g�b�o��O���F��K���Ӹ�multi-thread decode���X.
    if( switch_subtitle_flag == true )
    {
        switch_subtitle_flag    =   false;
        if( s_decoder.get_sub_src_type() == SubSourceType::FROM_FILE )
            s_decoder.switch_subtltle(new_subtitle_path);
        else if( s_decoder.get_sub_src_type() == SubSourceType::EMBEDDED )
            s_decoder.switch_subtltle(new_subtitle_index);
        else
            MYLOG( LOG::ERROR, "no subtitle.");
    }

    int     ret     =   0;

    // ������ subtitle �i��decode, ���M filter �|�X��
    if( s_decoder.find_index( pkt->stream_index ) == true )
    {
        ret     =   s_decoder.decode_subtitle(pkt);
        return  ret;
    }

    // handle video stream with subtitle
    if( s_decoder.exist_stream() == true && 
        s_decoder.is_graphic_subtitle() == false &&
        v_decoder.find_index( pkt->stream_index ) == true )
    {
        decode_video_with_nongraphic_subtitle(pkt);
        return  SUCCESS;
    }

    // handle video and audio
    VideoData   vdata;
    AudioData   adata;

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
                if( s_decoder.exist_stream() == true && s_decoder.is_graphic_subtitle() == true )
                    vdata   =   overlap_subtitle_image();
                else
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
Player::decode_video_with_subtitle()
********************************************************************************/
int    Player::decode_video_with_nongraphic_subtitle( AVPacket* pkt )
{
    int         ret         =   v_decoder.send_packet(pkt);
    AVFrame     *v_frame    =   nullptr;
    VideoData   vdata;
    int64_t     ts;

    int         count       =   0;      // for test.


    if( ret >= 0 )
    {
        while(true)
        {
            ret     =   v_decoder.recv_frame(pkt->stream_index);
            if( ret <= 0 )
                break;

            /*
                �o�� filter �� function �令���| keep ������
                �ҥH���������o ts, ���M�����|�]�� filter �ᥢ�h timestamp ��T.
                �p�G���b subtitle ����,�|�ݭn�����p�ƻs video stream �� time_base �L�h.
                ı�o�ӳ·ФF,��ܥΥ����X timestamp ���@�k
                ���I�O�o��{���X�����涶�Ǫ����D.
                �n�d�N while loop ����⦸�����p
            */
            v_frame     =   v_decoder.get_frame();
            ts          =   v_decoder.get_timestamp();
            ret         =   s_decoder.send_video_frame( v_frame );

            count       =   0;
            while(true) // note: �ثe���յ��G���O�@���@�i,���T�w�O�_�s�b�@����i��case
            {
                ret     =   s_decoder.render_subtitle();
                if( ret <= 0 )
                    break;

                vdata.frame         =   s_decoder.get_subtitle_image();
                vdata.index         =   v_decoder.get_frame_count();
                vdata.timestamp     =   ts;

                v_mtx.lock();
                video_queue.push(vdata);
                v_mtx.unlock();

                s_decoder.unref_frame();

                count++;
            }

            if( count > 1 )
                MYLOG( LOG::ERROR, "count > 1. %d", count );  // �T�{�O�_�X�{ loop �]�⦸�� case.

            v_decoder.unref_frame();
        }
    }

    return  SUCCESS;
}






/*******************************************************************************
Player::flush()

flush �L�{�򥻤W�P decode, �e nullptr �i�h
�]�|�R�X�@�i frame, �ݭn�N�� frame ��Ƽg�J output.

********************************************************************************/
int    Player::flush()
{
    int         ret     =   0;
    VideoData   vdata;
    AudioData   adata;

    // �B�zsubtitle, �D�Ϥ�.
    auto    flush_with_nongraphic_subtitle  =   [this] () -> VideoData
    {
        VideoData   vd;
        int         ret;
        AVFrame     *frame  =   v_decoder.get_frame();

        // �{���o�����Ӥ��ݭn�j�鱱��, ���d�N�O�_�|�X�{�@�i frame render �X�h�i�Ϥ����{�H.
        ret =   s_decoder.send_video_frame( frame );
        if( ret < 0 )
            MYLOG( LOG::ERROR, "flush send fail." );

        ret =   s_decoder.render_subtitle();
        if( ret < 0 )
            MYLOG( LOG::ERROR, "flush render fail." );
        
        vd.frame         =   s_decoder.get_subtitle_image();
        vd.index         =   v_decoder.get_frame_count();
        vd.timestamp     =   v_decoder.get_timestamp();

        s_decoder.unref_frame();

        return  vd;
    };


    // flush subtitle
    // �ݭn�Ҽ{ graphic �� case
    s_decoder.flush_all_stream();       

    // flush video
    ret     =   v_decoder.send_packet(nullptr);
    if( ret >= 0 )
    {       
        while(true)
        {
            ret     =   v_decoder.recv_frame(-1);
            if( ret <= 0 )
                break;

            if( s_decoder.exist_stream() == true )
            {
                if( s_decoder.is_graphic_subtitle() == true )
                    vdata   =   overlap_subtitle_image();
                else                
                    vdata   =   flush_with_nongraphic_subtitle();                
            }
            else
                vdata   =   v_decoder.output_video_data();

            // flush ���q���ӷQ�B�z subtitle, ���|�����~, �٨S���ѨM�����k. �ثe�u�B�zgraphic subtitle������
            v_mtx.lock();
            video_queue.push(vdata);
            v_mtx.unlock();

            v_decoder.unref_frame();  
        }
    }
    v_decoder.flush_all_stream();

    // flush audio
    ret     =   a_decoder.send_packet(nullptr);
    if( ret >= 0 )
    {
        while(true)
        {
            ret     =   a_decoder.recv_frame(-1);
            if( ret <= 0 )
                break;

            adata   =   a_decoder.output_audio_data();

            a_mtx.lock();
            audio_queue.push(adata);
            a_mtx.unlock();

            a_decoder.unref_frame();
        }
    }
    a_decoder.flush_all_stream();

    return 0;
}






/*******************************************************************************
Player::end()
********************************************************************************/
void    Player::clear_setting()
{
    setting.filename.clear();
    setting.subname.clear();
}





/*******************************************************************************
Player::end()
********************************************************************************/
int     Player::end()
{
    clear_setting();

    v_decoder.end();
    a_decoder.end();
    s_decoder.end();
    //subname.clear();

    stop_flag   =   false;
    seek_flag   =   false;

    //
    if( demuxer == nullptr )
        MYLOG( LOG::ERROR, "demuxer is null." );
    demuxer->end();
    delete demuxer;
    demuxer =   nullptr;

    return  SUCCESS;
}




/*******************************************************************************
Player::switch_subtitle()
********************************************************************************/
void    Player::switch_subtitle( std::string path )
{
    switch_subtitle_flag    =   true;
    new_subtitle_path       =   path;
}





/*******************************************************************************
Player::switch_subtitle()
********************************************************************************/
void    Player::switch_subtitle( int index )
{
    switch_subtitle_flag    =   true;
    new_subtitle_index      =   index;
}







/*******************************************************************************
Player::overlap_subtitle_image()
********************************************************************************/
VideoData       Player::overlap_subtitle_image()
{
    int64_t     timestamp   =   v_decoder.get_timestamp();
    VideoData   vdata;

    if( s_decoder.is_video_in_duration( timestamp ) == false )
        vdata     =   v_decoder.output_video_data();
    else
    {
        QImage  v_img   =   v_decoder.get_video_image();
        QImage  s_img   =   s_decoder.get_subtitle_image();
        
        QPainter    painter( &v_img );
        QPoint      pos =   s_decoder.get_subtitle_image_pos();
        painter.drawImage( pos, s_img );

        vdata.frame     =   v_img;
        vdata.index     =   v_decoder.get_frame_count();
        vdata.timestamp =   timestamp;
    }

    return vdata;
}





/*******************************************************************************
Player::seek()
********************************************************************************/
void    Player::seek( int value, int old_value )
{
    seek_flag   =   true;
    seek_value  =   value;
    seek_old    =   old_value;
}




#ifdef FFMPEG_TEST
/*******************************************************************************
player_decode_example
********************************************************************************/
void    player_decode_example()
{
    DecodeSetting   setting;
    setting.io_type     =   IO_Type::DEFAULT;
    setting.filename   =   "D:/input.avi";
    setting.subname    =   "D:/input.mkv";


    Player  player;  

    player.set( setting );
    player.init();

    player.set_output_jpg_root( "H:\\jpg" );
    player.set_output_audio_pcm_path( "H:\\test.pcm" );

    player.play();
    player.end();

}
#endif
