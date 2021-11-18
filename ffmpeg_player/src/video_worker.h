#ifndef VIDEO_WORKER_H
#define VIDEO_WORKER_H


#include <QThread>
#include <QAudioOutput>
#include <QImage>

#include "player.h"
#include "tool.h"

#include <thread>


class QMutex;


class VideoWorker : public QThread
{
    Q_OBJECT

public:

    VideoWorker( QObject *parent );
    ~VideoWorker();

    void run() override;

    void video_play();


public slots:


signals:
    
    void	recv_video_frame_signal();

private:

    Player  player;

    bool v_start = false;
    bool a_start = false;

    QMutex*     video_mtx   =   nullptr;    // ¸ò mainwindows ¦@¥Î

};



#endif