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
{
    // note: 目前只支援 stream output. 有需要的話再增加錄影用的介面.
    maker   =   create_maker_io();
}




/*******************************************************************************
Worker::~Worker()
********************************************************************************/
Worker::~Worker()
{
    delete  maker;
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
Worker::set_output()
********************************************************************************/
void    Worker::set_output( bool enable, std::string _port )
{
    is_output   =   enable;
    port        =   _port.empty() == false ? _port : "1234";
}







/*******************************************************************************
Worker::set_type()
********************************************************************************/
void    Worker::set_type( WorkType _t )
{
    wtype    =   _t;
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
    switch( wtype )
    {
    case WorkType::DEFAULT :
        setting.io_type     =   IO_Type::DEFAULT;
        setting.filename    =   filename;
        setting.subname     =   subname;
        break;
    case WorkType::SRT :
        setting.io_type     =   IO_Type::SRT_IO;
        setting.srt_ip      =   ip;
        setting.srt_port    =   port;
        break;
    default:
        assert(0);
    }

    //
    player.set(setting);
    player.init();

    int     duration    =   static_cast<int>(player.get_duration_time());
    emit    duration_signal( duration );

    if( player.is_embedded_subtitle() == true )
    {
        auto    list    =   player.get_embedded_subtitle_list();
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
#ifdef USE_MT
    player.play_QT_multi_thread();
#else
    if( is_output == true )
        player.play_live_stream();
    else
        player.play_QT();
#endif
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
Worker::output()
********************************************************************************/
void    Worker::output( MediaInfo media_info )
{
    MYLOG( LOG::INFO, "enable output." );
    output_by_io( media_info, port, maker );
}






/*******************************************************************************
Worker::run()
********************************************************************************/
void    Worker::run()  
{
    play_init(); // 為了取得 file 資訊, 將 play init 拆開來    

    // note: wtype = default 代表從檔案讀取
    // 目前暫不支援從 live stream output.
    if( is_output == true && wtype == WorkType::DEFAULT )
    {
        MediaInfo   media_info  =   player.get_media_info();

        player.add_audio_frame_cb   =   std::bind( &encode::add_audio_frame, std::placeholders::_1 );
        player.add_video_frame_cb   =   std::bind( &encode::add_video_frame, std::placeholders::_1 );

        if( output_thr != nullptr )
            MYLOG( LOG::ERROR, "output_thr not null." );
        output_thr  =   new std::thread( &Worker::output, this, media_info );

        while( maker->is_connect() == false )
            SLEEP_10MS;
    }

    play();

    if( output_thr != nullptr && wtype == WorkType::DEFAULT )
    {
        output_thr->join();
        delete  output_thr;
        output_thr  =   nullptr;
    }
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






/*******************************************************************************
Worker::seek_slot()
********************************************************************************/
void    Worker::seek_slot( int value )
{
    MainWindow  *mw     =   dynamic_cast<MainWindow*>(parent());
    VideoData   *vd     =   mw->get_view_data();
    int     old_value   =   vd->timestamp / 1000;

    player.seek( value, old_value );
}
