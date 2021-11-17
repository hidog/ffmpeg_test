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

    Worker();
    ~Worker();

    void run() override;

    void get_video_frame( QImage frame );
    void get_audio_pcm( AudioData ad );

    void audio_play();
    void video_play();

    void decode();

    std::function<void(QImage*)> main_cb;

public slots:
    void handleStateChanged( QAudio::State state );


signals:
    
    void	recv_video_frame_signal();

private:

    Player  player;

    QAudioOutput *audio = nullptr;
    QIODevice    *io = nullptr;

    std::thread *thr_decode;
    std::thread *thr_audio_play;
    std::thread *thr_video_paly;

    bool v_start = false;
    bool a_start = false;

};



#endif