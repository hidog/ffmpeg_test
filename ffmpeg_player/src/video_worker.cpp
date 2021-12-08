#include "video_worker.h"

#include <QDebug>
#include <QMutex>

#include "player.h"

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
    force_stop  =   false;
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

seek ���ɭ�, �� UI �ݭt�d�M�Ÿ��, ���ᵥ�즳��Ƥ~�~�򼽩�.
�� player �M�Ū���, �|�J�� queue �O�_���Ū��P�_���n�g, 
�S�g�n�|�y�� handle_func crash.
********************************************************************************/
void    VideoWorker::flush_for_seek()
{
    std::queue<VideoData>   *v_queue    =   get_video_queue();

    bool    &a_start    =   dynamic_cast<MainWindow*>(parent())->get_audio_worker()->get_audio_start_state();

    // ���s���ݦ���Ƥ~����
    while( v_queue->size() <= 3 )
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
    // �o��S��print, �|�y�� release build �L�@��
    MYLOG( LOG::INFO, "start play video" );

    std::mutex &v_mtx   =   get_v_mtx();

    std::chrono::steady_clock::time_point       last, now;
    std::chrono::duration<int64_t, std::milli>  duration;

    bool    &ui_v_seek_lock = get_v_seek_lock();


    std::queue<VideoData>*  v_queue     =   get_video_queue();
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
    while( v_queue->size() <= 3 )    
        SLEEP_10MS;
    v_start     =   true;
    while( a_start == false )
        SLEEP_10MS;

    //
    auto    handle_func =   [&]() 
    {
        v_mtx.lock();
        VideoData vd    =   v_queue->front();
        v_queue->pop();
        v_mtx.unlock();

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
        emit    update_seekbar_signal( view_data->timestamp / 1000 );

        last    =   now;
    };

    //
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
            ui_v_seek_lock  =   true;
            while( ui_v_seek_lock == true )
                SLEEP_10MS;
            flush_for_seek();
        }

        //
        if( v_queue->size() <= 0 )
        {
            MYLOG( LOG::WARN, "video queue empty." );
            SLEEP_10MS;
            continue;
        }

        handle_func();
    }

    // flush
    while( v_queue->empty() == false && force_stop == false )
    {       
        while( pause_flag == true )
            SLEEP_10MS;

        handle_func();
    }

    // �� player ����, �T�O���|�A�W�[��ƶi�hqueue
    while( is_play_end == false )
        SLEEP_10MS;

    // force stop ���ɭԻݭn��ʲM�����
    while( v_queue->empty() == false )
        v_queue->pop();
}





/*******************************************************************************
VideoWorker::pause()
********************************************************************************/
void    VideoWorker::pause()
{
    pause_flag  =   !pause_flag;
}
