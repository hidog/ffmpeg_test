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

public slots:

signals:    
    void	recv_video_frame_signal();

private:
    bool        v_start     =   false;
    QMutex*     video_mtx   =   nullptr;    // �� mainwindows �@��
    bool        force_stop  =   false;
    bool        pause_flag  =   false;
};



#endif