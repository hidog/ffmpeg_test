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
    void    audio_play();
    bool&   get_audio_start_state();

    int     get_volume();
    void    stop();
    void    pause();

    void    flush_for_seek();    
    void    set_only_audio( bool flag );
    void    update_seekbar( int sec );

    const int&  get_current_sec();

public slots:

    void    handleStateChanged( QAudio::State state );
    void    volume_slot( int value );
    void    seek_slot( int sec );

signals:

    void    update_seekbar_signal( int sec );

private:

    QAudioOutput    *audio     =   nullptr;
    QIODevice       *io        =   nullptr;

    bool    a_start     =   false;
    bool    force_stop  =   false;
    bool    seek_flag   =   false;
    bool    only_audio  =   false;
    int     current_sec =   0;
};



#endif