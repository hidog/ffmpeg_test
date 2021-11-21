#include "video_worker.h"

#include <QDebug>
#include <QMutex>

#include "mainwindow.h"
#include "player.h"
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
VideoWorker::get_video_start_state()
********************************************************************************/
bool&   VideoWorker::get_video_start_state()
{
    return  v_start;
}




/*******************************************************************************
VideoWorker::~VideoWorker()
********************************************************************************/
VideoWorker::~VideoWorker()
{}




/*******************************************************************************
VideoWorker::run()
********************************************************************************/
void VideoWorker::run()  
{
    video_play();

    MYLOG( LOG::INFO, "finish video play." );
}




#include "player.h"


/*******************************************************************************
VideoWorker::video_play()
********************************************************************************/
void VideoWorker::video_play()
{
    std::mutex &v_mtx = get_v_mtx();
    // 這邊沒有print, 會造成 release build 無作用
    MYLOG( LOG::INFO, "start play video" );

    std::chrono::steady_clock::time_point       start, now;
    std::chrono::duration<int64_t, std::milli>  duration;

    std::queue<VideoData>*  v_queue     =   get_video_queue();
    VideoData               *view_data  =   dynamic_cast<MainWindow*>(parent())->get_view_data();

    bool    &a_start        =   dynamic_cast<MainWindow*>(parent())->get_audio_worker()->get_audio_start_state();
    bool    &is_play_end    =   dynamic_cast<MainWindow*>(parent())->get_worker()->get_play_end_state();

    if( view_data == nullptr )
        MYLOG( LOG::ERROR, "view_data is null." );

    //
    while( v_queue->size() <= 3 )    
        SLEEP_10MS;
    v_start     =   true;
    while( a_start == false )
        SLEEP_10MS;

    // need start timestamp for control.
    start   =   std::chrono::steady_clock::now();

    //
    while( is_play_end == false )
    {       
        if( v_queue->size() <= 0 )
        {
            MYLOG( LOG::WARN, "video queue empty." );
            SLEEP_10MS;
            continue;
        }

        v_mtx.lock();
        VideoData vd    =   v_queue->front();
        v_queue->pop();
        v_mtx.unlock();


        while(true)
        {
            now         =   std::chrono::steady_clock::now();
            duration    =   std::chrono::duration_cast<std::chrono::milliseconds>( now - start );

            if( duration.count() >= vd.timestamp )
                break;
        }

        video_mtx->lock();

        view_data->index        =   vd.index;
        view_data->frame        =   vd.frame;
        view_data->timestamp    =   vd.timestamp;

        video_mtx->unlock();
            
        emit recv_video_frame_signal();
    }

    // flush
    while( v_queue->empty() == false )
    {       
        VideoData vd    =   v_queue->front();
        v_queue->pop();

        while(true)
        {
            now         =   std::chrono::steady_clock::now();
            duration    =   std::chrono::duration_cast<std::chrono::milliseconds>( now - start );

            if( duration.count() >= vd.timestamp )
                break;
        }

        video_mtx->lock();

        view_data->index        =   vd.index;
        view_data->frame        =   vd.frame;
        view_data->timestamp    =   vd.timestamp;

        video_mtx->unlock();

        emit recv_video_frame_signal();
    }
}

