#ifndef WORKER_H
#define WORKER_H


#include <QThread>
#include <QMutex>
#include <thread>

#include "player/player.h"
#include "maker/maker_interface.h"



enum class WorkType
{
    DEFAULT     =   0,          // load from file.
    SRT,
};




class Worker : public QThread
{
    Q_OBJECT

public:
    
    Worker( QObject *parent );
    ~Worker();
    
    void    run() override;   

    void    play_init();
    void    play();
    void    end();

    bool&   get_play_end_state();

    void    set_src_file( std::string file );
    bool    is_set_src_file();
    void    finish_set_video();

    void    set_type( WorkType _t );
    void    set_ip( std::string _ip );
    void    set_port( std::string _port );

    void    play_video_audio();
    void    play_audio();
    
    // use for output    
    void    set_output( bool enable, std::string _port );
    void    output( MediaInfo media_info );

    QStringList     get_subtitle_files( std::string filename );

public slots:

    void    switch_subtitle_slot_str( QString path );
    void    switch_subtitle_slot_int( int index );
    void    stop_slot();
    void    finish_slot();
    void    seek_slot( int value );


signals:
    void    video_setting_signal( VideoDecodeSetting );
    void    subtitle_list_signal( QStringList );
    void    embedded_sublist_signal( std::vector<std::string> );
    void    duration_signal(int);

private:

    WorkType    wtype    =   WorkType::DEFAULT;

    Player  *player     =   nullptr;
    bool    is_set_video    =   false;
    bool    is_play_end     =   true;

    std::string     filename;
    std::string     subname;

    std::string     ip;
    std::string     port;

    // use for output. 
    std::thread*    output_thr  =   nullptr;  // 方便測試, 先這樣寫. 日後重構
    bool            is_output   =   false;

    // use for encode and output.
    MakerInterface*     maker   =   nullptr;

    QMutex  end_lock;
};



#endif