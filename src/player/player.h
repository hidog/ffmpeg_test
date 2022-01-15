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
//#include "../IO/input_output.h"

#include <QImage>

DLL_API std::queue<AudioData>* get_audio_queue();
DLL_API std::queue<VideoData>* get_video_queue();

DLL_API std::mutex& get_a_mtx(); 
DLL_API std::mutex& get_v_mtx(); 

DLL_API bool& get_v_seek_lock(); 
DLL_API bool& get_a_seek_lock(); 


struct AVPacket;


#ifdef FFMPEG_TEST
//#define RENDER_SUBTITLE  // 是否要將字幕加進video frame內
#endif



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
    void    stop();
    void    seek( int value, int old_value );
    void    set( DecodeSetting _setting );

    //
    bool    demux_need_wait();
    bool    is_set_input_file();
    int     decode( Decode *dc, AVPacket* pkt );
    //void    set_sub_file( std::string str );
    int     init_demuxer();

    int     decode_video_with_nongraphic_subtitle( AVPacket* pkt );
    void    switch_subtitle( std::string path );
    void    switch_subtitle( int index );
    bool    is_embedded_subtitle();
    bool    is_file_subtitle();

    void    init_subtitle( AVFormatContext *fmt_ctx );
    void    handle_seek();    
    void    clear_setting();    

    int64_t         get_duration_time();
    VideoData       overlap_subtitle_image();

    VideoDecodeSetting    get_video_setting();
    AudioDecodeSetting    get_audio_setting();

    std::vector<std::string>    get_embedded_subtitle_list();

#ifdef USE_MT
    void    play_QT_multi_thread();
    void    video_decode();
    void    audio_decode();
#endif

#ifdef FFMPEG_TEST
    void    play(); 
    void    play_decode_video_subtitle( AVPacket* pkt );
    std::function<void(QImage)> output_video_frame_func;
    std::function<void(AudioData)> output_audio_pcm_func;

    void    set_output_jpg_root( std::string _root_path );
    void    set_output_audio_pcm_path( std::string _path );
#else
    MediaInfo   get_media_info();   // use for output.
#endif
     
    // use for live steeam
    void    init_live_stream();
    void    end_live_stream();
    void    play_live_stream();
    void    output_live_stream( Decode* dc );

    AVFrame*    get_new_v_frame();
    AVFrame*    get_new_a_frame();

    std::function< void(AVFrame*) >     add_audio_frame_cb;
    std::function< void(AVFrame*) >     add_video_frame_cb;

private:

    //static constexpr int   MAX_QUEUE_SIZE  =   50;
    static constexpr int   MAX_QUEUE_SIZE  =   10;


    Demux*          demuxer     =   nullptr;
    VideoDecode     v_decoder;
    AudioDecode     a_decoder;
    SubDecode       s_decoder;

    //std::string     src_file;
    //std::string     sub_name;           // 外掛字幕檔名
    std::string     new_subtitle_path;  // switch subtitle使用

    bool    switch_subtitle_flag    =   false;
    int     new_subtitle_index      =   0;
    bool    stop_flag               =   false;

    int     seek_old    =   0;
    int     seek_value  =   0;
    bool    seek_flag   =   false;

#ifdef USE_MT
    bool    v_thr_start     =   false,
        a_thr_start     =   false;

    std::queue<AVPacket*>   video_pkt_queue,
        audio_pkt_queue;

    std::thread     *video_decode_thr   =   nullptr,
        *audio_decode_thr   =   nullptr;
#endif

    DecodeSetting   setting;

    // use for live stream.
    bool        is_live_stream      =   false;
    int64_t     audio_pts_count     =   0;
};




int     io_read_data( void *opaque, uint8_t *buf, int buf_size );



#ifdef FFMPEG_TEST
void    player_decode_example();
#endif



#endif