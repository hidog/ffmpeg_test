#ifndef VIDEO_WORKER_H
#define VIDEO_WORKER_H


#include <QThread>
#include "tool.h"


class QMutex;


class VideoWorker : public QThread
{
    Q_OBJECT

public:

    VideoWorker( QObject *parent );
    ~VideoWorker();

    void    run() override;
    void    video_play();
    bool&   get_video_start_state();
    void    stop();
    void    pause();

    void    flush_for_seek();    

public slots:

    void    seek_slot( int sec );

signals:    
    
    void	recv_video_frame_signal();
    void    update_seekbar_signal( int sec );

private:

    bool        v_start     =   false;
    QMutex*     video_mtx   =   nullptr;    // ¸ò mainwindows ¦@¥Î
    bool        force_stop  =   false;
    bool        pause_flag  =   false;

    bool        seek_flag   =   false;
};



#endif