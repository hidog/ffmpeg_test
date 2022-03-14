#include "play_worker.h"

#include <QDebug>
#include <QDir>

#include "music_worker.h"
#include "ui/mainwindow.h"

#include "tool.h"





/*******************************************************************************
PlayWorker::PlayWorker()
********************************************************************************/
PlayWorker::PlayWorker( QObject *parent )
    :   QThread(parent)
{
    player  =   std::make_unique<Player>();
}





/*******************************************************************************
PlayWorker::~PlayWorker()
********************************************************************************/
PlayWorker::~PlayWorker()
{}




/*******************************************************************************
PlayWorker::play_init()
********************************************************************************/
void    PlayWorker::play_init()
{
    DecodeSetting   setting;
    setting.io_type     =   IO_Type::DEFAULT;
    setting.filename    =   filename;

    //
    player->set( setting );
    player->init();

    int     duration    =   static_cast<int>(player->get_duration_time());
    emit    duration_signal( duration );
}








/*******************************************************************************
PlayWorker::play()
********************************************************************************/
void    PlayWorker::play()
{
    AudioDecodeSetting    as;
    MusicWorker     *mw     =   dynamic_cast<MainWindow*>(parent())->get_music_worker();
    
    // send audio setting to UI
    as  =   player->get_audio_setting();
    mw->open_audio_output(as);

    //
    is_play_end     =   false;
    mw->start();
    
    player->play_QT();
    player->end();

    is_play_end     =   true;
    
    //while( mw->isRunning() == true )
      //  SLEEP_10MS;
    //mw->wait( QDeadlineTimer(QDeadlineTimer::Forever) );
    bool flag   =   mw->wait( 100000 );
    if( flag == false )
    {
        MYLOG( LOG::L_ERROR, "timeout." )
    }

    MYLOG( LOG::L_INFO, "finish decode." )
}





/*******************************************************************************
PlayWorker::run()
********************************************************************************/
void    PlayWorker::run()  
{
    play_init(); // 為了取得 file 資訊, 將 play init 拆開來    
    play();
}





/*******************************************************************************
PlayWorker::end()
********************************************************************************/
void    PlayWorker::end()
{
    filename.clear();
}






/*******************************************************************************
PlayWorker::stop_slot()
********************************************************************************/
void    PlayWorker::stop_slot()
{
    end_lock.lock();
    player->stop();
    end_lock.unlock();

    MusicWorker     *mw     =   dynamic_cast<MainWindow*>(parent())->get_music_worker();
    mw->stop();
}






/*******************************************************************************
PlayWorker::set_src_file()
********************************************************************************/
void    PlayWorker::set_src_file( std::string file )
{
    filename    =   file;
}





/*******************************************************************************
PlayWorker::is_set_src_file()
********************************************************************************/
bool    PlayWorker::is_set_src_file()
{
    return  player->is_set_input_file();
}





/*******************************************************************************
PlayWorker::seek_slot()
********************************************************************************/
void    PlayWorker::seek_slot( int value )
{
    MusicWorker *mw     =   dynamic_cast<MainWindow*>(parent())->get_music_worker();
    int     old_value   =   mw->get_timestamp() / 1000; // sec.
    player->seek( value, old_value );
}





/*******************************************************************************
PlayWorker::get_play_end_state()
********************************************************************************/
bool&   PlayWorker::get_play_end_state()
{
    return  is_play_end;
}
