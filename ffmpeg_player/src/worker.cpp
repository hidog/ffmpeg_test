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
    VideoSetting    vs;
    AudioSetting    as;
    AudioWorker     *aw     =   dynamic_cast<MainWindow*>(parent())->get_audio_worker();
    VideoWorker     *vw     =   dynamic_cast<MainWindow*>(parent())->get_video_worker();

    player.init();

    //
    is_set_video    =   false;
    vs              =   player.get_video_setting();
    emit video_setting_singal(vs);

    //
    as  =   player.get_audio_setting();
    aw->open_audio_output(as);

    while( is_set_video == false )
        SLEEP_10MS;

    //
    aw->start();
    vw->start();

    //
    player.play_QT();
    player.end();

    MYLOG( LOG::INFO, "finish decode." );
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
