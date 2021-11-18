#ifndef WORKER_H
#define WORKER_H


#include <QThread>
#include <QAudioOutput>
#include <QImage>

#include "player.h"
#include "tool.h"

#include <thread>


class Worker : public QThread
{
    Q_OBJECT

public:

    Worker( QObject *parent );
    ~Worker();

    void run() override;

    void decode();

    std::function<void(QImage*)> main_cb;

public slots:


signals:
    
    void	recv_video_frame_signal();

private:

    Player  player;


    bool v_start = false;
    bool a_start = false;

};



#endif