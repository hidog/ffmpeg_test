#ifndef AUDIO_WORKER_H
#define AUDIO_WORKER_H


#include <QThread>
#include <QAudioOutput>
#include <QImage>

#include "player.h"
#include "tool.h"

#include <thread>


class AudioWorker : public QThread
{
    Q_OBJECT

public:

    AudioWorker( QObject *parent );
    ~AudioWorker();

    //
    void    run() override;

    //
    void    open_audio_output( AudioSetting as );
    void    audio_play();

public slots:
    void handleStateChanged( QAudio::State state );

signals:


private:

    QAudioOutput *audio     =   nullptr;
    QIODevice    *io        =   nullptr;

    bool v_start = false;
    bool a_start = false;

};



#endif