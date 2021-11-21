#include "worker.h"

#include <QDebug>

#include "audio_worker.h"
#include "video_worker.h"

#include "mainwindow.h"



/*******************************************************************************
Worker::Worker()
********************************************************************************/
Worker::Worker( QObject *parent )
    :   QThread(parent)
{}




/*******************************************************************************
Worker::~Worker()
********************************************************************************/
Worker::~Worker()
{}







/*******************************************************************************
Worker::run()
********************************************************************************/
void    Worker::run()  
{
    //while(true) {

        VideoSetting    vs;
        AudioSetting    as;
        AudioWorker     *aw     =   dynamic_cast<MainWindow*>(parent())->get_audio_worker();
        VideoWorker     *vw     =   dynamic_cast<MainWindow*>(parent())->get_video_worker();

        player.init();

        // send video setting to UI
        is_set_video    =   false;
        vs              =   player.get_video_setting();
        emit video_setting_singal(vs);

        // send audio setting to UI
        as  =   player.get_audio_setting();
        aw->open_audio_output(as);

        while( is_set_video == false )
            SLEEP_10MS;

        // need implement, send subtitle setting to UI.

        //
        is_play_end     =   false;
        aw->start();
        vw->start();

        //
        //player.play_QT_multi_thread();
        player.play_QT();
        player.end();
        is_play_end     =   true;

        MYLOG( LOG::INFO, "finish decode." );


        // 應該要做處理,判斷其他兩個thread結束後,才跑下一個loop
        // 暫時先省略       
        //std::this_thread::sleep_for( std::chrono::seconds(10) );
    //}
}




/*******************************************************************************
Worker::get_play_end_state()
********************************************************************************/
bool&   Worker::get_play_end_state()
{
    return  is_play_end;
}




/*******************************************************************************
Worker::finish_set_video()
********************************************************************************/
void    Worker::finish_set_video()
{
    is_set_video    =   true;
}








/*******************************************************************************
Worker::set_src_file()
********************************************************************************/
void    Worker::set_src_file( std::string file )
{
    player.set_input_file(file);
}




/*******************************************************************************
Worker::is_set_src_file()
********************************************************************************/
bool    Worker::is_set_src_file()
{
    return  player.is_set_input_file();
}
