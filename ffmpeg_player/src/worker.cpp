#include "worker.h"

#include <QDebug>
#include <QDir>

#include "audio_worker.h"
#include "video_worker.h"
#include "mainwindow.h"
#include "tool.h"




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
{
    if( player != nullptr )
    delete  player;
}




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
    filter << "*.ass" << "*.ssa" << "*.srt" << "*.sub";
    //filter.push_back("*.ass");
    //filter.push_back("*.ssa");
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
Worker::set_ip()
********************************************************************************/
void    Worker::set_ip( std::string _ip )
{
    ip  =   _ip;
}



/*******************************************************************************
Worker::set_type()
********************************************************************************/
void    Worker::set_port( std::string _port )
{
    port    =   _port;
}





/*******************************************************************************
Worker::play_init()
********************************************************************************/
void    Worker::play_init()
{
    DecodeSetting   setting;    

    setting.filename    =   filename;
    setting.subname     =   subname;

    //
    player->set( setting );
    player->init();

    int     duration    =   static_cast<int>(player->get_duration_time());
    emit    duration_signal( duration );

    if( player->is_embedded_subtitle() == true )
    {
        auto    list    =   player->get_embedded_subtitle_list();
        emit embedded_sublist_signal(list);
    }
}








/*******************************************************************************
Worker::play()
********************************************************************************/
void    Worker::play()
{
    VideoDecodeSetting    vs;
    AudioDecodeSetting    as;
    AudioWorker     *aw     =   dynamic_cast<MainWindow*>(parent())->get_audio_worker();
    VideoWorker     *vw     =   dynamic_cast<MainWindow*>(parent())->get_video_worker();

    if( player->exist_video_stream() == true )
    {
        aw->set_only_audio( false );    

        // send video setting to UI
        is_set_video    =   false;
        vs              =   player->get_video_setting();
        emit video_setting_signal(vs);

        // wait for setting video.
        while( is_set_video == false )
            SLEEP_10MS;
    }
    else
        aw->set_only_audio( true );
    
    // send audio setting to UI
    as  =   player->get_audio_setting();
    aw->open_audio_output(as);

    //
    is_play_end     =   false;
    aw->start();

    if( player->exist_video_stream() == true )    
        vw->start();
    else
        vw->set_no_stream();
    
    //
#ifdef USE_MT
    player.play_QT_multi_thread();
#else
    player->play_QT();
#endif

    player->end();
    is_play_end     =   true;
    
    // 等待其他兩個thread完成
    while( aw->isFinished() == false )
        SLEEP_10MS;
    while( vw->isFinished() == false && player->exist_video_stream() == true )
        SLEEP_10MS;

    if( player->exist_video_stream() == false )
        decode::clear_video_queue(); // 音檔有封面的時候,要清掉

    MYLOG( LOG::L_INFO, "finish decode." );
}





/*******************************************************************************
Worker::run()
********************************************************************************/
void    Worker::run()  
{
    // 確保沒在其他地方被初始化
    assert( player == nullptr );

    player  =   new Player; // 未來有需要的話, 增加 create_player(type)
    assert( player != nullptr );

    play_init(); // 為了取得 file 資訊, 將 play init 拆開來    
    play();
}



/*******************************************************************************
Worker::stop_slot()
********************************************************************************/
void    Worker::end()
{
    end_lock.lock();
    if( player != nullptr )
    {
        delete  player;
        player  =   nullptr;
    }
    end_lock.unlock();
}






/*******************************************************************************
Worker::stop_slot()
********************************************************************************/
void    Worker::stop_slot()
{
    end_lock.lock();
    if( player != nullptr )
        player->stop();
    end_lock.unlock();

    AudioWorker     *aw     =   dynamic_cast<MainWindow*>(parent())->get_audio_worker();
    VideoWorker     *vw     =   dynamic_cast<MainWindow*>(parent())->get_video_worker();

    vw->stop();
    aw->stop();
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

    filename    =   file;
    //player.set_input_file(file);
    auto list   =   get_subtitle_files(file);

    if( list.size() > 0 )
    {
        str         =   list.at(0);
        //player.set_sub_file( str.toStdString() ); // 未來做成可以多重輸入
        subname     =   str.toStdString();

        emit subtitle_list_signal(list);
    }
}




/*******************************************************************************
Worker::is_set_src_file()
********************************************************************************/
bool    Worker::is_set_src_file()
{
    return  player->is_set_input_file();
}





/*******************************************************************************
Worker::switch_subtitle()
********************************************************************************/
void    Worker::switch_subtitle_slot_str( QString path )
{
    if( player != nullptr )
    {
        if( player->is_file_subtitle() == true )
            player->switch_subtitle( path.toStdString() );
    }
}




/*******************************************************************************
Worker::switch_subtitle()
********************************************************************************/
void    Worker::switch_subtitle_slot_int( int index )
{
    if( player != nullptr )
    {
        if( player->is_embedded_subtitle() == true )
            player->switch_subtitle(index);
    }
}






/*******************************************************************************
Worker::seek_slot()
********************************************************************************/
void    Worker::seek_slot( int value )
{
    // 播放純音檔的時候應該要從 audio_worker 讀取 old_value. 但暫時不影響執行結果, 先跳過.
    MainWindow  *mw     =   dynamic_cast<MainWindow*>(parent());
    VideoData   *vd     =   mw->get_view_data();
    int     old_value   =   vd->timestamp / 1000;

    player->seek( value, old_value );
}





/*******************************************************************************
Worker::get_play_end_state()
********************************************************************************/
bool&   Worker::get_play_end_state()
{
    return  is_play_end;
}
