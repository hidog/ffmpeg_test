#include "player.h"

#include "tool.h"
#include "demux_io.h"
#include "video_decode.h"
#include "audio_decode.h"


#include <thread>



extern "C" {

#include <libavformat/avformat.h>

} // end extern "C"





/*******************************************************************************
Player::init_demuxer()
********************************************************************************/
int     Player::init_demuxer()
{
    if( demuxer != nullptr )
        MYLOG( LOG::L_ERROR, "demuxer not null." );
    if( setting.io_type == IO_Type::DEFAULT )
    {
        demuxer     =   std::make_unique<Demux>();
        if( setting.filename.empty() == true )
        {
            MYLOG( LOG::L_ERROR, "src file is empty." );
            return  R_ERROR;   
        }
        demuxer->set_input_file( setting.filename );
    }
    else
    {
        demuxer     =   std::make_unique<DemuxIO>(setting);
        if( demuxer == nullptr )
        {
            MYLOG( LOG::L_ERROR, "init demuxer fail." );
            return  R_ERROR;
        }
    }

    return  R_SUCCESS;
}





#ifndef FFMPEG_TEST
/*******************************************************************************
Player::get_media_info()
********************************************************************************/
MediaInfo   Player::get_media_info()
{
    MediaInfo   info;
    AVStream*   stream  =   nullptr;

    // video
    if( decode_manager->exist_video_stream() == true )
    {
        VideoDecode     *v_dec  =   decode_manager->get_current_video_decoder();
        info.width      =   v_dec->get_video_width();
        info.height     =   v_dec->get_video_height();
        info.pix_fmt    =   v_dec->get_pix_fmt();
    
        stream  =   v_dec->get_stream();
        if( stream != nullptr )
        {
            info.time_num   =   stream->avg_frame_rate.den;
            info.time_den   =   stream->avg_frame_rate.num;
        }
        else
            MYLOG( LOG::L_ERROR, "stream is nullptr" );
    }

    // audio
    if( decode_manager->exist_audio_stream() == true )
    {
        AudioDecode     *a_dec  =   decode_manager->get_current_audio_decoder();
        info.channel_layout =   a_dec->get_audio_channel_layout();
        info.sample_rate    =   a_dec->get_audio_sample_rate();
        info.sample_fmt     =   a_dec->get_audio_sample_format();
    }

    return  info;
}
#endif





/*******************************************************************************
Player::init()
********************************************************************************/
int     Player::init()
{
    int     ret{0};

    // init demux
    ret     =   init_demuxer();
    if( ret != R_SUCCESS )
    {
        MYLOG( LOG::L_ERROR, "init demux fail." );
        return  ret;
    }

    //
    stop_flag   =   false;
    seek_flag   =   false;

    AVFormatContext *fmt_ctx    =   nullptr;

    //
    ret     =   demuxer->init();
    ret     =   demuxer->open_input();
    fmt_ctx =   demuxer->get_format_context();

    //
    decode_manager->open_decoders( fmt_ctx );
    decode_manager->init_decoders();

    // handle subtitle
    // 有遇到影片會在這邊卡很久, 或許可以考慮用multi-thread的方式做處理, 以後再說...
    decode_manager->init_subtitle( fmt_ctx, setting );

    return R_SUCCESS;
}









#ifdef FFMPEG_TEST
/*******************************************************************************
Player::set_output_openCV_jpg_root()
********************************************************************************/
void    Player::set_output_jpg_path( std::string _path )
{
    decode_manager->set_output_jpg_path(_path);
}
#endif







#ifdef FFMPEG_TEST
/*******************************************************************************
Player::set_output_audio_pcm_path()
********************************************************************************/
void    Player::set_output_audio_pcm_path( std::string _path )
{
    decode_manager->set_output_audio_pcm_path(_path);
}
#endif





/*******************************************************************************
Player::get_setting
********************************************************************************/
DecodeSetting&  Player::get_setting()
{
    return  setting;
}




/*******************************************************************************
Player::get_muxer
********************************************************************************/
Demux*  Player::get_demuxer()
{
    return  demuxer.get();
}





/*******************************************************************************
Player::Player()
********************************************************************************/
Player::Player()
{
    decode_manager  =   std::make_unique<DecodeManager>();
}




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
    vs.width    =   0;
    vs.height   =   0;

    if( decode_manager->exist_video_stream() == true )
    {
        VideoDecode     *v_ptr  =   decode_manager->get_current_video_decoder();
        vs.width    =   v_ptr->get_video_width();
        vs.height   =   v_ptr->get_video_height();
        vs.code_id  =   v_ptr->get_video_codec();

        vs.time_base_num    =   v_ptr->get_video_time_base_num();
        vs.time_base_den    =   v_ptr->get_video_time_base_den();
        
        vs.pix_fmt  =   static_cast<int>(v_ptr->get_pix_fmt());
    }

    return  vs;
}





/*******************************************************************************
Player::get_audio_setting()
********************************************************************************/
AudioDecodeSetting    Player::get_audio_setting()
{
    AudioDecodeSetting    as;
    as.channel      =   0;
    as.sample_rate  =   0;

    if( decode_manager->exist_audio_stream() == true )
    {
        AudioDecode     *a_ptr  =   decode_manager->get_current_audio_decoder();
        as.channel      =   a_ptr->get_audio_channel();
        as.sample_rate  =   a_ptr->get_audio_sample_rate();
        as.sample_size  =   a_ptr->get_audeo_sample_size();
        as.sample_type  =   a_ptr->get_audio_sample_type();
        as.code_id      =   a_ptr->get_audio_code_id();

        as.channel_layout   =   a_ptr->get_audio_channel_layout();
    }
    return  as;
}




/*******************************************************************************
Player::get_duration_time
********************************************************************************/
int64_t     Player::get_duration_time()
{
    if( demuxer == nullptr )  
        MYLOG( LOG::L_ERROR, "demuxer is null." );
    return  demuxer->get_duration_time();
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
    if( demuxer == nullptr )
        MYLOG( LOG::L_ERROR, "demuxer is null." );

    int         ret     =   0;
    AVPacket    *pkt    =   nullptr;
    Decode      *dc     =   nullptr;

    // read frames from the file 
    while( true ) 
    {
        ret     =   demuxer->demux();
        if( ret < 0 )        
            break;        
        
        pkt     =   demuxer->get_packet();
        dc      =   decode_manager->get_decoder( pkt->stream_index );

        // send
        ret =   dc->send_packet(pkt);
        if( ret >= 0 )
        {
            // recv
            while(true)
            {
                ret =   dc->recv_frame(pkt->stream_index);
                if( ret <= 0 )
                    break;
            
                if( dc->output_frame_func != nullptr && dc->get_is_current() == true )
                    dc->output_frame_func();
                dc->unref_frame();
            }
            
        }
        demuxer->unref_packet();
    }
    MYLOG( LOG::L_INFO, "play main loop end. it will flush." );

    // flush
    decode_manager->flush_decoders();
    MYLOG( LOG::L_INFO, "Demuxing finish." )
}
#endif





/*******************************************************************************
Player::get_new_v_frame()
********************************************************************************/
AVFrame*    Player::get_new_v_frame()
{
    VideoDecode     *v_ptr  =   get_decode_manager()->get_current_video_decoder();

    AVFrame*    v_frame     =   nullptr;
    int         ret         =   0;
    
    //
    v_frame   =   av_frame_alloc();
    if( v_frame == nullptr ) 
        MYLOG( LOG::L_ERROR, "v_frame alloc fail." );
    v_frame->pts  =   0;

    //
    v_frame->format   =   v_ptr->get_pix_fmt();
    v_frame->width    =   v_ptr->get_video_width();
    v_frame->height   =   v_ptr->get_video_height();

    ret     =   av_frame_get_buffer( v_frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "get buffer fail." );

    ret     =   av_frame_make_writable(v_frame);
    if( ret < 0 )
        assert(0);

    return  v_frame;
}




/*******************************************************************************
Player::output_video_frame_to_encode()
********************************************************************************/
void    Player::output_video_frame_to_encode( Decode* dc )
{
    AVFrame*    frame   =   nullptr;
    AVFrame*    v_frame =   nullptr;

    if( dc->get_decode_context_type() == AVMEDIA_TYPE_VIDEO )
    {
        if( encode::is_video_queue_full() == true )
            return;

        v_frame     =   get_new_v_frame();
        frame       =   dc->get_frame();
        av_frame_copy( v_frame, frame );
        v_frame->pts =   dc->get_frame_count();        
        add_video_frame_cb(v_frame);
    }
    else
    {
        MYLOG( LOG::L_ERROR, "decode type is not video" );
        assert(0);
    }
}








#ifdef FFMPEG_TEST
/*******************************************************************************
Player::play_decode_video()
********************************************************************************/
void    Player::play_decode_video()
{
    if( demuxer == nullptr )
        MYLOG( LOG::L_ERROR, "demuxer is null." );

    output_live_stream_func     =   std::bind( &Player::output_video_frame_to_encode, this, std::placeholders::_1 );
    add_video_frame_cb          =   std::bind( &encode::add_video_frame, std::placeholders::_1 );

    int         ret     =   0;
    AVPacket    *pkt    =   nullptr;
    Decode      *dc     =   nullptr;
    VideoData   vdata;

    //
    auto video_need_wait    =   [this] () 
    {
        if( decode_manager->exist_video_stream() == true && decode::get_video_size() >= MAX_QUEUE_SIZE )
            return  true;  
        else
            return  false;
    };

    // read frames from the file 
    while( true ) 
    {
        while( video_need_wait() == true )        
            SLEEP_10MS;        

        ret     =   demuxer->demux();
        if( ret < 0 )        
            break;
        
        pkt     =   demuxer->get_packet();

        if( pkt->stream_index == decode_manager->get_current_video_index() )
        {
            dc  =   decode_manager->get_decoder( pkt->stream_index );

            // send
            ret =   dc->send_packet(pkt);
            if( ret >= 0 )
            {
                // recv
                while(true)
                {
                    ret =   dc->recv_frame(pkt->stream_index);
                    if( ret <= 0 )
                        break;            
                    
                    output_live_stream_func( dc );                    
                    dc->unref_frame();
                }
            
            }
        }

        demuxer->unref_packet();
    }
    MYLOG( LOG::L_INFO, "play main loop end. it will flush." );

    // flush
    decode_manager->flush_decoders();
    MYLOG( LOG::L_INFO, "Demuxing finish." )
}
#endif






#ifdef FFMPEG_TEST
/*******************************************************************************
Player::play_output_audio()
********************************************************************************/
void    Player::play_output_audio()
{
    if( demuxer == nullptr )
        MYLOG( LOG::L_ERROR, "demuxer is null." );

    int         ret     =   0;
    AVPacket    *pkt    =   nullptr;
    Decode      *dc     =   nullptr;

    // read frames from the file 
    while( true ) 
    {
        ret     =   demuxer->demux();
        if( ret < 0 )        
            break;
        
        pkt     =   demuxer->get_packet();

        if( pkt->stream_index == decode_manager->get_current_audio_index() )
        {
            dc  =   decode_manager->get_decoder( pkt->stream_index );

            // send
            ret =   dc->send_packet(pkt);
            if( ret >= 0 )
            {
                // recv
                while(true)
                {
                    ret =   dc->recv_frame(pkt->stream_index);
                    if( ret <= 0 )
                        break;
            
                    if( dc->output_frame_func != nullptr && dc->get_is_current() == true )
                        dc->output_frame_func();
                    dc->unref_frame();
                }
            
            }
        }

        demuxer->unref_packet();
    }
    MYLOG( LOG::L_INFO, "play main loop end. it will flush." );

    // flush
    decode_manager->flush_decoders();
    MYLOG( LOG::L_INFO, "Demuxing finish." )
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

    while( v_thr_start == false || a_thr_start == false )
        SLEEP_10MS;

    while( true ) 
    {
        while( video_queue.size() > 100 || audio_queue.size() > 200 )      // 1080p, 10bit的影像, 必須拉大buffer
        {
            SLEEP_1MS;  // 這邊不能睡太久, 不然會造成 demux 速度來不及.
            MYLOG( LOG::L_DEBUG, "v queue %d, a queue %d", video_queue.size(), audio_queue.size() );
        }

        auto    pkt_pair    =   demuxer.demux_multi_thread();
        ret     =   pkt_pair.first;
        pkt     =   pkt_pair.second;
        if( ret < 0 )
            break;    
        if( pkt == nullptr )
        {
            MYLOG( LOG::L_ERROR, "pkt is nullptr" );
            break;
        }

        if( v_decoder.find_index( pkt->stream_index ) == true )
            video_pkt_queue.push(pkt);
        else if( a_decoder.find_index( pkt->stream_index ) == true )
            audio_pkt_queue.push(pkt);       
        else
        {
            MYLOG( LOG::L_DEBUG, "stream type not handle.")
            demuxer.collect_packet(pkt);
        }

        SLEEP_1MS;
    }

    //
    //flush();
    MYLOG( LOG::L_INFO, "play finish.")

    video_decode_thr->join();
    audio_decode_thr->join();

    delete video_decode_thr; video_decode_thr = nullptr;
    delete audio_decode_thr; audio_decode_thr = nullptr;
}
#endif







/*******************************************************************************
Player::demux_need_wait()

為了避免 queue 被塞爆,量過大的時候會停下來等 UI 端消化.
要考慮只有video or audio stream.
********************************************************************************/
bool    Player::demux_need_wait()
{
    if( seek_flag == true )
        return  false;
    else if( decode_manager->exist_video_stream() == true && decode_manager->exist_audio_stream() == true )
    {
        if( decode_manager->is_video_attachd() == true ) // 處理音樂檔案內有封面的case
        {
            if( decode::get_audio_size() >= MAX_QUEUE_SIZE )
                return  true;
            else
                return  false;
        }
        else if( decode::get_video_size() >= MAX_QUEUE_SIZE && decode::get_audio_size() >= MAX_QUEUE_SIZE )
            return  true;
        else
            return  false;
    }   
    else if( decode_manager->exist_video_stream() == true && decode::get_video_size() >= MAX_QUEUE_SIZE )
        return  true;  
    else if( decode_manager->exist_audio_stream() == true && decode::get_audio_size() >= MAX_QUEUE_SIZE )
        return  true;
    else
        return  false;
}





/*******************************************************************************
Player::is_embedded_subtitle()
********************************************************************************/
bool    Player::is_embedded_subtitle()
{
    return  decode_manager->get_sub_src_type() == SubSourceType::EMBEDDED;
}






/*******************************************************************************
Player::is_file_subtitle()
********************************************************************************/
bool    Player::is_file_subtitle()
{
    return  decode_manager->get_sub_src_type() == SubSourceType::FROM_FILE;
}





/*******************************************************************************
Player::is_embedded_subtitle()
********************************************************************************/
std::vector<std::string>    Player::get_embedded_subtitle_list()
{
    return  decode_manager->get_embedded_subtitle_list();
}





/*******************************************************************************
Player::stop()
********************************************************************************/
void    Player::stop()
{
    stop_flag   =   true;
}





/*******************************************************************************
Player::stop()
********************************************************************************/
bool    Player::get_stop_flag()
{
    return  stop_flag;
}




/*******************************************************************************
Player::handle_seek()

看討論, avformat_seek_file 比 av_seek_frame 好
但細節需要研究.
********************************************************************************/
void    Player::handle_seek()
{
    int         ret;
    AudioData   adata;

    // clear queue.
    decode::clear_video_queue();
    decode::clear_audio_queue();

    decode_manager->flush_decoders_for_seek();

    // run seek.
    AVFormatContext*    fmt_ctx     =   demuxer->get_format_context();
    avformat_flush( fmt_ctx );  // 看起來是走網路才需要做這個動作,但不確定

    int64_t     so  =   seek_old,
                sv  =   seek_value; // 避免overflow

    // 不這樣設置的話, seek大範圍會出錯.  例如超過一小時的影片, 直接 seek 超過半小時後
    int64_t     min     =   so < sv ? so * AV_TIME_BASE + 2 : INT64_MIN,
                max     =   so > sv ? so * AV_TIME_BASE - 2 : INT64_MAX,
                ts      =   sv * AV_TIME_BASE;       

    ret     =   avformat_seek_file( fmt_ctx, -1, min, ts, max, 0 );  //AVSEEK_FLAG_ANY
    if( ret < 0 )
        MYLOG( LOG::L_DEBUG, "seek fail." );

}




/*******************************************************************************
Player::play_QT()
********************************************************************************/
void    Player::play_QT()
{
    int         ret     =   0;
    AVPacket    *pkt    =   nullptr;
    Decode      *dc     =   nullptr;

    // 簡單的 sync 控制. 有空再修
    bool&   ui_v_seek_lock  =   decode::get_v_seek_lock();
    bool&   ui_a_seek_lock  =   decode::get_a_seek_lock();

    //
    while( stop_flag == false ) 
    {
        //MYLOG( LOG::L_DEBUG, "video = %d, audio = %d", video_queue.size(), audio_queue.size() );
        // NOTE: seek事件觸發的時候, queue 資料會暴增.
        while( demux_need_wait() == true )
        {
            //MYLOG( LOG::L_DEBUG, "v size = %d, a size = %d", video_queue.size(), audio_queue.size() );
            if( stop_flag == true )
                break;
            SLEEP_1MS;
        }

        // NOTE: 如果 demux 已經結束, 此時無法seek.
        if( seek_flag == true )     
        {
            if( decode_manager->exist_video_stream() == true && decode_manager->is_video_attachd() == false )
            {
                while( ui_v_seek_lock == false || ui_a_seek_lock == false )
                    SLEEP_10MS;
                seek_flag   =   false;
                handle_seek();
                ui_v_seek_lock = false;
                ui_a_seek_lock = false;
            }
            else
            {
                while( ui_a_seek_lock == false )
                    SLEEP_10MS;
                seek_flag   =   false;
                handle_seek();
                ui_a_seek_lock = false;
            }
        }

        //
        ret     =   demuxer->demux();
        if( ret < 0 )
            break;

        //
        pkt     =   demuxer->get_packet();
        dc      =   decode_manager->get_decoder( pkt->stream_index );

        //
        decode( dc, pkt );       
        demuxer->unref_packet();
    }
    MYLOG( LOG::L_INFO, "demux finish.");

    //
    flush();
    MYLOG( LOG::L_INFO, "play finish.")
}






/*******************************************************************************
Player::decode
********************************************************************************/
int     Player::decode( Decode *dc, AVPacket* pkt )
{
    // switch subtitle track
    // 寫在這邊是為了方便未來跟 multi-thread decode 結合.
    if( switch_subtitle_flag == true )
    {
        switch_subtitle_flag    =   false;
        if( decode_manager->get_sub_src_type() == SubSourceType::FROM_FILE )
            decode_manager->switch_subtltle(new_subtitle_path);
        else if( decode_manager->get_sub_src_type() == SubSourceType::EMBEDDED )
            decode_manager->switch_subtltle(new_subtitle_index);
        else
            MYLOG( LOG::L_ERROR, "no subtitle.");
    }

    int ret =   0;

    // handle video and audio
    VideoData   vdata;
    AudioData   adata;

    ret =   dc->send_packet(pkt);
    if( ret >= 0 )
    {
        while(true)
        {
            ret     =   dc->recv_frame(pkt->stream_index);
            if( ret <= 0 )
                break;

            if( output_live_stream_func != nullptr )
                output_live_stream_func( dc );

            //if( pkt->stream_index == v_decoder.current_index() )
            if( pkt->stream_index == decode_manager->get_current_video_index() )
            {
                VideoDecode *v_ptr  =   decode_manager->get_current_video_decoder();
                vdata   =   v_ptr->output_video_data();
                decode::add_video_data(vdata);                
            }
            //else if( pkt->stream_index == a_decoder.current_index() )
            else if( pkt->stream_index == decode_manager->get_current_audio_index() )
            {
                AudioDecode *a_ptr  =   decode_manager->get_current_audio_decoder();
                adata   =   a_ptr->output_audio_data();
                decode::add_audio_data(adata);
            }
            else if( decode_manager->index_is_available( pkt->stream_index ) )
            {
                // no nothing.
            }
            else 
            {
                MYLOG( LOG::L_ERROR, "undefined behavier." );
            }
            dc->unref_frame();
        }
    }

    return  R_SUCCESS;
}








/*******************************************************************************
Player::flush()

flush 過程基本上同 decode, 送 nullptr 進去
也會吐出一張 frame, 需要將此 frame 資料寫入 output.

********************************************************************************/
int    Player::flush()
{
    int         ret     =   0;
    VideoData   vdata;
    AudioData   adata;

    // flush subtitle
    // 需要考慮 graphic 的 case
    decode_manager->flush_all_sub_stream();
   
    // flush video
    if( decode_manager->exist_video_stream() == true )
    {
        VideoDecode     *v_ptr  =   decode_manager->get_current_video_decoder();
        ret     =   v_ptr->send_packet(nullptr);
        if( ret >= 0 )
        {       
            while(true)
            {
                ret     =   v_ptr->recv_frame(-1);
                if( ret <= 0 )
                    break;

                if( output_live_stream_func != nullptr )
                    output_live_stream_func( v_ptr );

                vdata   =   v_ptr->output_video_data();
                decode::add_video_data(vdata);  

                v_ptr->unref_frame();  
            }
        }
        decode_manager->flush_video();
    }

    // flush audio
    if( decode_manager->exist_audio_stream() == true )
    {
        AudioDecode     *a_ptr  =   decode_manager->get_current_audio_decoder();
        ret     =   a_ptr->send_packet(nullptr);
        if( ret >= 0 )
        {
            while(true)
            {
                ret     =   a_ptr->recv_frame(-1);
                if( ret <= 0 )
                    break;

                if( output_live_stream_func != nullptr )
                    output_live_stream_func( a_ptr );

                adata   =   a_ptr->output_audio_data();
                decode::add_audio_data( adata );
                a_ptr->unref_frame();
            }
        }
        decode_manager->flush_audio();
    }

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

    //stop_flag   =   false;
    seek_flag   =   false;

    //
    if( demuxer == nullptr )
        MYLOG( LOG::L_ERROR, "demuxer is null." );
    demuxer->end();
    demuxer.release();

    decode_manager->release();

    return  R_SUCCESS;
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
    //setting.io_type     =   IO_Type::SRT_IO;
    setting.filename   =   "D:\\test_video\\test.mkv";     // 使用 D:\\code\\test.mkv 會出錯. 已增加程式碼處理這個問題.
    //setting.subname    =   "D:\\test.ass";   
    //setting.srt_port    =   "1234";

    Player  player;  

    player.set( setting );
    player.init();

    player.set_output_jpg_path( "F:\\jpg" );
    player.set_output_audio_pcm_path( "F:\\test.pcm" );

    player.play();
    player.end();

}
#endif





/*******************************************************************************
Player::exist_video_stream()
********************************************************************************/
bool    Player::exist_video_stream()
{
    return  decode_manager->exist_video_stream() == true && decode_manager->is_video_attachd() == false;
}





/*******************************************************************************
Player::get_decode_manager()
********************************************************************************/
DecodeManager*  Player::get_decode_manager()
{
    return  decode_manager.get();
}
