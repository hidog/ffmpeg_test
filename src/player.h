#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <stdio.h>
#include <queue>

#include "demux.h"
#include "audio_decode.h"
#include "video_decode.h"
#include "tool.h"

#include <QImage>



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
    void    play_QT();
    int     init();
    int     end();
    int     flush();

    //
    void    set_input_file( std::string path );
    bool    is_set_input_file();

    VideoSetting    get_video_setting();
    AudioSetting    get_audio_setting();


    //
    std::function<void(QImage)> output_video_frame_func;
    std::function<void(AudioData)> output_audio_pcm_func;

    //
#ifdef FFMPEG_TEST
    void    play();
#endif

private:

    Demux           demuxer;
    VideoDecode     v_decoder;
    AudioDecode     a_decoder;

    std::string     src_filename;
};





#endif