#ifndef PLAY_WORKER_H
#define PLAY_WORKER_H


#include <QThread>
#include <QMutex>
#include <thread>
#include <memory>

#include "player/player.h"



class PlayWorker : public QThread
{
    Q_OBJECT

public:
    
    PlayWorker( QObject *parent );
    ~PlayWorker();
    
    void    run() override;   

    void    play_init();
    void    play();
    void    end();

    bool&   get_play_end_state();
    void    set_src_file( std::string file );
    bool    is_set_src_file();

public slots:

    void    stop_slot();
    void    seek_slot( int value );

signals:

    void    duration_signal(int);

private:

    std::unique_ptr<Player> player;
    bool    is_play_end     =   true;
    std::string     filename;
    QMutex  end_lock;
};



#endif