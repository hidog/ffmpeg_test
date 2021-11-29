#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <stdio.h>
#include <queue>
#include <thread>
#include <mutex>

#include "demux.h"
#include "audio_decode.h"
#include "video_decode.h"
#include "sub_decode.h"
#include "tool.h"

#include <QImage>

DLL_API std::queue<AudioData>* get_audio_queue();
DLL_API std::queue<VideoData>* get_video_queue();

DLL_API std::mutex& get_a_mtx(); 
DLL_API std::mutex& get_v_mtx(); 


struct AVPacket;



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
    bool    demux_need_wait();
    void    set_input_file( std::string path );
    bool    is_set_input_file();
    int     decode( Decode *dc, AVPacket* pkt );
    void    set_sub_file( std::string str );
    int     decode_video_with_subtitle( AVPacket* pkt );

    VideoSetting    get_video_setting();
    AudioSetting    get_audio_setting();


#ifdef USE_MT
    void    play_QT_multi_thread();
    void    video_decode();
    void    audio_decode();
#endif

#ifdef FFMPEG_TEST
    void    play();
    std::function<void(QImage)> output_video_frame_func;
    std::function<void(AudioData)> output_audio_pcm_func;
#endif

private:

    static constexpr int   MAX_QUEUE_SIZE  =   50;

    Demux           demuxer;
    VideoDecode     v_decoder;
    AudioDecode     a_decoder;
    SubDecode       s_decoder;

    std::string     src_file;
    std::string     sub_name;   // 外掛字幕檔名
    //std::string     sub_src;    // 因為可能是使用內嵌字幕,也可能用外掛字幕. 將最後的結果存在這個字串內.

#ifdef USE_MT
    bool    v_thr_start     =   false,
        a_thr_start     =   false;

    std::queue<AVPacket*>   video_pkt_queue,
        audio_pkt_queue;

    std::thread     *video_decode_thr   =   nullptr,
        *audio_decode_thr   =   nullptr;
#endif

};





#endif