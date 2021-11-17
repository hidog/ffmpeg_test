#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <stdio.h>

#include "demux.h"
#include "audio_decode.h"
#include "video_decode.h"

#include <QImage>

#include <queue>
#include "tool.h"



DLL_API std::queue<AudioData>* get_audio_queue();
DLL_API std::queue<VideoData>* get_video_queue();





class DLL_API Player
{
public:
    Player();
    ~Player();

    Player( const Player& ) = delete;
    Player( Player&& ) = delete;

    Player& operator = ( const Player& ) = delete;
    Player& operator = ( Player&& ) = delete;

    //
    void    play();
    void    play_QT();
    int     init();
    int     end();

    void    pause();
    void    resume();

    std::function<void(QImage)> output_video_frame_func;
    std::function<void(AudioData)> output_audio_pcm_func;

private:

    Demux           demuxer;
    VideoDecode     v_decoder;
    AudioDecode     a_decoder;

    std::string src_filename, 
                video_dst_filename, 
                audio_dst_filename;

    int video_stream_idx    =   -1, 
        audio_stream_idx    =   -1;

    int width, 
        height;

    FILE    *audio_dst_file =   NULL;

    int video_frame_count   =   0;
    int audio_frame_count   =   0;

    bool    is_pause    =   false;
};





#endif