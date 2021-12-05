#include "worker.h"

#include <QDebug>
#include <QDir>

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
Worker::get_subtitle_files()
********************************************************************************/
QStringList Worker::get_subtitle_files( std::string filename )
{
    QString     qstr { filename.c_str() };
    QDir        dir { qstr };

    int     len             =   dir.dirName().size() - dir.dirName().indexOf('.');
    QString filename_only   =   dir.dirName().chopped(len);

    dir.cdUp();

    //
    QStringList     filter;
    filter.push_back("*.ass");
    filter.push_back("*.ssa");
    //filter.push_back("*.sub");

    //
    QStringList     sub_list;
    QString         subname;
    auto    list    =   dir.entryInfoList( filter );
    for( auto itr : list )
    {
        len     =   itr.fileName().size() - itr.fileName().indexOf('.');
        subname =   itr.fileName().chopped(len);

        if( subname == filename_only )
            sub_list.push_back( itr.absoluteFilePath() );
    }

    return sub_list;
}





/*******************************************************************************
Worker::run()
********************************************************************************/
void    Worker::run()  
{
    VideoSetting    vs;
    AudioSetting    as;
    AudioWorker     *aw     =   dynamic_cast<MainWindow*>(parent())->get_audio_worker();
    VideoWorker     *vw     =   dynamic_cast<MainWindow*>(parent())->get_video_worker();
   
    //
    player.init();
    int     duration    =   static_cast<int>(player.get_duration_time());
    emit    duration_signal( duration );

    if( player.is_embedded_subtitle() == true )
    {
        auto    list    =   player.get_embedded_subtitle_list();
        emit embedded_sublist_signal(list);
    }
    
    // send video setting to UI
    is_set_video    =   false;
    vs              =   player.get_video_setting();
    emit video_setting_signal(vs);
    
    // send audio setting to UI
    as  =   player.get_audio_setting();
    aw->open_audio_output(as);
    
    // wait for setting video.
    while( is_set_video == false )
        SLEEP_10MS;
    
    //
    is_play_end     =   false;
    aw->start();
    vw->start();
    
    //
    //player.play_QT_multi_thread();
    player.play_QT();
    player.end();
    is_play_end     =   true;
    
    // 等待其他兩個thread完成
    while( aw->isFinished() == false )
        SLEEP_10MS;
    while( vw->isFinished() == false )
        SLEEP_10MS;

    MYLOG( LOG::INFO, "finish decode." );
}




/*******************************************************************************
Worker::stop_slot()
********************************************************************************/
void    Worker::stop_slot()
{
    player.stop();

    AudioWorker     *aw     =   dynamic_cast<MainWindow*>(parent())->get_audio_worker();
    VideoWorker     *vw     =   dynamic_cast<MainWindow*>(parent())->get_video_worker();

    vw->stop();
    aw->stop();
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
    QString     str;

    player.set_input_file(file);
    auto list   =   get_subtitle_files(file);

    if( list.size() > 0 )
    {
        str     =   list.at(0);
        player.set_sub_file( str.toStdString() ); // 未來做成可以多重輸入

        emit subtitle_list_signal(list);
    }
}




/*******************************************************************************
Worker::is_set_src_file()
********************************************************************************/
bool    Worker::is_set_src_file()
{
    return  player.is_set_input_file();
}





/*******************************************************************************
Worker::switch_subtitle()
********************************************************************************/
void    Worker::switch_subtitle_slot_str( QString path )
{
    if( player.is_file_subtitle() == true )
        player.switch_subtitle( path.toStdString() );
}




/*******************************************************************************
Worker::switch_subtitle()
********************************************************************************/
void    Worker::switch_subtitle_slot_int( int index )
{
    if( player.is_embedded_subtitle() == true )
        player.switch_subtitle(index);
}
