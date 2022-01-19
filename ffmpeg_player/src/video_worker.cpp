#include "video_worker.h"

#include <QDebug>
#include <QMutex>

#include "player/player.h"

#include "mainwindow.h"
#include "audio_worker.h"
#include "worker.h"




/*******************************************************************************
VideoWorker::VideoWorker()
********************************************************************************/
VideoWorker::VideoWorker( QObject *parent )
    :   QThread(parent)
{
    video_mtx   =   dynamic_cast<MainWindow*>(parent)->get_video_mutex();
    if( video_mtx == nullptr )    
        MYLOG( LOG::ERROR, "video mtx is null" );
}




/*******************************************************************************
VideoWorker::~VideoWorker()
********************************************************************************/
VideoWorker::~VideoWorker()
{}




/*******************************************************************************
VideoWorker::get_video_start_state()
********************************************************************************/
bool&   VideoWorker::get_video_start_state()
{
    return  v_start;
}








/*******************************************************************************
VideoWorker::run()
********************************************************************************/
void VideoWorker::run()  
{
    force_stop  =   false;
    seek_flag   =   false;
    video_play();
    MYLOG( LOG::INFO, "finish video play." );
}






/*******************************************************************************
VideoWorker::stop()
********************************************************************************/
void    VideoWorker::stop()
{
    force_stop  =   true;
}





/*******************************************************************************
VideoWorker::seek_slot()
********************************************************************************/
void    VideoWorker::seek_slot( int sec )
{
    seek_flag   =   true;
    v_start     =   false;
}





/*******************************************************************************
VideoWorker::flush_for_seek()

seek 的時候, 由 UI 端負責清空資料, 之後等到有資料才繼續播放.
由 player 清空的話, 會遇到 queue 是否為空的判斷不好寫, 
沒寫好會造成 handle_func crash.
********************************************************************************/
void    VideoWorker::flush_for_seek()
{
    bool    &a_start    =   dynamic_cast<MainWindow*>(parent())->get_audio_worker()->get_audio_start_state();

    // 重新等待有資料才播放
    while( decode::get_video_size() <= 3 )
        SLEEP_10MS;
    v_start     =   true;
    while( a_start == false )
        SLEEP_10MS;
}




/*******************************************************************************
VideoWorker::video_play()
********************************************************************************/
void VideoWorker::video_play()
{
    // 這邊沒有print, 會造成 release build 無作用
    MYLOG( LOG::INFO, "start play video" );

    std::chrono::steady_clock::time_point       last, now;
    std::chrono::duration<int64_t, std::milli>  duration;

    VideoData               *view_data  =   dynamic_cast<MainWindow*>(parent())->get_view_data();    

    // init again.
    view_data->index        =   0;
    view_data->timestamp    =   0;

    //
    bool    &a_start        =   dynamic_cast<MainWindow*>(parent())->get_audio_worker()->get_audio_start_state();
    bool    &is_play_end    =   dynamic_cast<MainWindow*>(parent())->get_worker()->get_play_end_state();

    if( view_data == nullptr )
        MYLOG( LOG::ERROR, "view_data is null." );

    //  
    while( decode::get_video_size() <= 3 )
        SLEEP_10MS;

    std::this_thread::sleep_for( std::chrono::milliseconds(350) );  // live stream 的時候, 降低延遲的 work around.

    v_start     =   true;
    while( a_start == false )
        SLEEP_10MS;

    //
    auto    handle_func =   [&]() 
    {
        VideoData vd    =   decode::get_video_data();

        while(true)
        {
            now         =   std::chrono::steady_clock::now();
            duration    =   std::chrono::duration_cast<std::chrono::milliseconds>( now - last );

            if( duration.count() >= vd.timestamp - view_data->timestamp )
                break;
        }

        video_mtx->lock();
        view_data->index        =   vd.index;
        view_data->frame        =   vd.frame;
        view_data->timestamp    =   vd.timestamp;
        video_mtx->unlock();

        emit    recv_video_frame_signal();

        if( seek_flag == false )
            emit    update_seekbar_signal( view_data->timestamp / 1000 );

        last    =   now;
    };

    // 用 bool 簡易做 sync, 有空再修
    bool    &ui_v_seek_lock     =   decode::get_v_seek_lock();

    last   =   std::chrono::steady_clock::now();
    while( is_play_end == false && force_stop == false )
    {   
        //
        while( pause_flag == true )
            SLEEP_10MS;

        //
        if( seek_flag == true )
        {
            seek_flag   =   false;
            last        =   std::chrono::steady_clock::time_point();
            view_data->timestamp    =   INT64_MAX;
            ui_v_seek_lock  =   true;
            while( ui_v_seek_lock == true )
                SLEEP_10MS;
            flush_for_seek();
        }

        //
        if( decode::get_video_size() <= 0 )
        {
            MYLOG( LOG::WARN, "video queue empty." );
            SLEEP_10MS;
            continue;
        }

        handle_func();
    }

    // flush
    while( decode::get_video_size() > 0 && force_stop == false )
    {       
        while( pause_flag == true )
            SLEEP_10MS;
        handle_func();
    }

    // 等 player 結束, 確保不會再增加資料進去queue
    while( is_play_end == false )
        SLEEP_10MS;

    // force stop 的時候需要手動清除資料
    decode::clear_video_queue();
}





/*******************************************************************************
VideoWorker::pause()
********************************************************************************/
void    VideoWorker::pause()
{
    pause_flag  =   !pause_flag;
}
