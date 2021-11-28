#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <stdio.h>
#include <queue>

#include "demux.h"
#include "audio_decode.h"
#include "video_decode.h"
#include "sub_decode.h"
#include "tool.h"

#include <QImage>
#include <thread>



DLL_API std::queue<AudioData>* get_audio_queue();
DLL_API std::queue<VideoData>* get_video_queue();

DLL_API std::mutex& get_a_mtx(); 
DLL_API std::mutex& get_v_mtx(); 


struct AVPacket;
struct AVStream;



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

#ifdef USE_MT
    void    play_QT_multi_thread();
    void    video_decode();
    void    audio_decode();
#endif

    //
    void    set_input_file( std::string path );
    bool    is_set_input_file();
    int     decode( Decode *dc, AVPacket* pkt );

    VideoSetting    get_video_setting();
    AudioSetting    get_audio_setting();


    void set_sub_file( std::string str );


    //
    std::function<void(QImage)> output_video_frame_func;
    std::function<void(AudioData)> output_audio_pcm_func;

    //
#ifdef FFMPEG_TEST
    void    play();
#endif

private:

    bool    v_thr_start     =   false,
            a_thr_start     =   false;


    Demux           demuxer;
    VideoDecode     v_decoder;
    AudioDecode     a_decoder;
    SubDecode       s_decoder;

    std::string     src_filename;

#ifdef USE_MT
    std::queue<AVPacket*>   video_pkt_queue,
                            audio_pkt_queue;

    std::thread     *video_decode_thr   =   nullptr,
                    *audio_decode_thr   =   nullptr;
#endif

    QImage  sub_img;


    std::string sub_name = "";


    int subtitleOpened = 0;

    //AVFilterGraph *filterGraph = nullptr;//avfilter_graph_alloc();

    //bool init_subtitle_filter( AVFilterContext * &buffersrcContext, AVFilterContext * &buffersinkContext, std::string args, std::string filterDesc);

};





#endif