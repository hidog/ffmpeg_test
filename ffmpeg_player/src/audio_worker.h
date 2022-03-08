#ifndef AUDIO_WORKER_H
#define AUDIO_WORKER_H

#include <QThread>
#include <QAudioOutput>
#include <QImage>

#include "player/player.h"
#include "tool.h"


class AudioWorker : public QThread
{
    Q_OBJECT

public:

    AudioWorker( QObject *parent );
    ~AudioWorker();
    
    void    run() override;

    void    open_audio_output( AudioDecodeSetting as );
    void    audio_play_with_video();
    void    audio_play();
    bool&   get_audio_start_state();

    int     get_volume();
    void    stop();
    void    pause();

    void    flush_for_seek();    
    void    set_only_audio( bool flag );

public slots:

    void    handleStateChanged( QAudio::State state );
    void    volume_slot( int value );
    void    seek_slot( int sec );

signals:

private:

    QAudioOutput    *audio     =   nullptr;
    QIODevice       *io        =   nullptr;

    bool    a_start     =   false;
    bool    force_stop  =   false;
    bool    seek_flag   =   false;
    bool    only_audio  =   false;

};



#endif