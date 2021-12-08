#include "player.h"
#include "tool.h"

#include <thread>
#include <QPainter>




extern "C" {

#include <libavcodec/avcodec.h>

} // end extern "C"


static std::queue<AudioData>    audio_queue;
static std::queue<VideoData>    video_queue;

std::mutex  a_mtx;
std::mutex  v_mtx;





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
Player::init()
********************************************************************************/
int     Player::init()
{
    if( src_file == "" )
    {
        MYLOG( LOG::ERROR, "src file not set." );
        return  ERROR;
    }

    stop_flag   =   false;

    int     ret     =   -1;
    AVFormatContext *fmt_ctx    =   nullptr;

    //
    ret     =   demuxer.open_input( src_file );
    ret     =   demuxer.init();
    fmt_ctx =   demuxer.get_format_context();

    //
    ret     =   v_decoder.open_codec_context( fmt_ctx );
    ret     =   a_decoder.open_codec_context( fmt_ctx );
    ret     =   s_decoder.open_codec_context( fmt_ctx );

    //
    ret     =   v_decoder.init();
    ret     =   a_decoder.init();    

    // handle subtitle
    init_subtitle(fmt_ctx);

    return SUCCESS;
}





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
        s_decoder.set_subfile( src_file );
        s_decoder.set_sub_src_type( SubSourceType::EMBEDDED );
    }
    else
    {
        if( sub_name.empty() == false )
        {
            exist_subtitle  =   true;
            s_decoder.set_subfile( sub_name );
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
            // 這邊有執行順序問題, 不能隨便更改執行順序      
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
void Player::set_sub_file( std::string str )
{
    sub_name    =   str;
}





/*******************************************************************************
Player::Player()
********************************************************************************/
void    Player::set_input_file( std::string path )
{
    src_file    =   path;
}




/*******************************************************************************
Player::is_set_input_file()
********************************************************************************/
bool    Player::is_set_input_file()
{
    return  src_file.size() != 0;
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




/*******************************************************************************
Player::get_duration_time
********************************************************************************/
int64_t     Player::get_duration_time()
{
    return  demuxer.get_duration_time();
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
        if( v_decoder.find_index(pkt->stream_index) )
            dc  =   dynamic_cast<Decode*>(&v_decoder);        
        else if( a_decoder.find_index(pkt->stream_index) )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else        
            MYLOG( LOG::WARN, "stream type not handle.");        

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
Player::demux_need_wait()

為了避免 queue 被塞爆,量過大的時候會停下來等 UI 端消化.
要考慮只有video or audio stream.

********************************************************************************/
bool    Player::demux_need_wait()
{
    if( v_decoder.exist_stream() == true && a_decoder.exist_stream() == true )
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
        while( demux_need_wait() == true )
        {
            if( stop_flag == true )
                break;
            SLEEP_1MS;
        }

        //
        ret     =   demuxer.demux();
        if( ret < 0 )
        {
            MYLOG( LOG::INFO, "demux finish.");
            break;
        }

        //
        pkt     =   demuxer.get_packet();
        if( v_decoder.find_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&v_decoder);
        else if( a_decoder.find_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else if( s_decoder.find_index( pkt->stream_index ) == true )
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
Player::decode
********************************************************************************/
int     Player::decode( Decode *dc, AVPacket* pkt )
{
    // switch subtitle track
    // 寫在這邊是為了方便未來跟multi-thread decode結合.
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

    // 必須對 subtitle 進行decode, 不然 filter 會出錯
    if( s_decoder.find_index( pkt->stream_index ) == true )
    {
        ret     =   s_decoder.decode_subtitle(pkt);
        return  ret;
    }

    // handle video stream with subtitle
    if( s_decoder.exist_stream() == true && s_decoder.is_graphic_subtitle() == false &&
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
                這邊 filter 的 function 改成不會 keep 的版本
                所以必須先取得 ts, 不然有機會跑完 filter 後失去 timestamp 資訊.
                如果做在 subtitle 的話,會需要視情況複製 video stream 的 time_base 過去.
                覺得太麻煩了,選擇用先取出 timestamp 的作法
                缺點是這邊程式碼有執行順序的問題.
                要留意 while loop 執行兩次的狀況
            */
            v_frame     =   v_decoder.get_frame();
            ts          =   v_decoder.get_timestamp();
            ret         =   s_decoder.send_video_frame( v_frame );

            count       =   0;
            while(true) // note: 目前測試結果都是一次一張,不確定是否存在一次兩張的case
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
                MYLOG( LOG::ERROR, "count > 1. %d", count );  // 確認是否出現 loop 跑兩次的 case.

            v_decoder.unref_frame();
        }
    }

    return  SUCCESS;
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

    // flush video
    ret     =   v_decoder.send_packet(nullptr);
    if( ret >= 0 )
    {       
        while(true)
        {
            ret     =   v_decoder.recv_frame(-1);
            if( ret <= 0 )
                break;

            if( s_decoder.exist_stream() == true && s_decoder.is_graphic_subtitle() == true )
                vdata   =   overlap_subtitle_image();
            else
                vdata   =   v_decoder.output_video_data();

            // flush 階段本來想處理 subtitle, 但會跳錯誤, 還沒找到解決的做法. 目前只處理graphic subtitle的部分
            video_queue.push(vdata);
            v_decoder.unref_frame();  
        }
    }
    v_decoder.flush_all_stresam();

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
    a_decoder.flush_all_stresam();

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

    sub_name.clear();

    stop_flag   =   false;

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

