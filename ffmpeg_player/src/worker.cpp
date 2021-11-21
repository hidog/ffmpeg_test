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
    QString qstr { filename.c_str() };

    QDir dir { qstr };

    qDebug() << dir.dirName();
    int len = dir.dirName().indexOf('.');
    QString filename_only = dir.dirName().chopped(len);

    qDebug() << filename_only;

    dir.cdUp();
    qDebug() << dir.dirName();

    QStringList filter;
    filter.push_back("*.ass");
    //filter.push_back("*.sub");


    QStringList sub_list;

    auto list = dir.entryInfoList( filter );
    for( auto itr : list )
    {
        qDebug() << itr.fileName();

        int len = itr.fileName().lastIndexOf('.');
        QString subname = itr.fileName().chopped(len);

        if( subname == filename_only )
            sub_list.push_back( itr.absoluteFilePath() );
    }

    for( auto itr : sub_list )
    {
        qDebug() << itr;
    }

    return sub_list;
}



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

    auto list = get_subtitle_files(file);

    player.set_sub_file(list[0].toStdString()); // 未來做成可以多重輸入
}




/*******************************************************************************
Worker::is_set_src_file()
********************************************************************************/
bool    Worker::is_set_src_file()
{
    return  player.is_set_input_file();
}
