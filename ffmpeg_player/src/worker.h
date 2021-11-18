#ifndef WORKER_H
#define WORKER_H


#include <QThread>
#include "player.h"


class Worker : public QThread
{
    Q_OBJECT

public:

    //
    Worker( QObject *parent );
    ~Worker();

    //
    void    run() override;

    //
    void    set_src_file( std::string file );
    bool    is_set_src_file();
    void    finish_set_video();
    bool&   get_play_end_state();

public slots:

    
signals:
    void    video_setting_singal( VideoSetting );


private:

    Player  player;
    bool    is_set_video    =   false;
    bool    is_play_end     =   true;

};



#endif