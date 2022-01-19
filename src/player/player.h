#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <stdio.h>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>

#include "demux.h"
#include "audio_decode.h"
#include "video_decode.h"
#include "sub_decode.h"
#include "tool.h"
//#include "../IO/input_output.h"

#include <QImage>




struct AVPacket;


#ifdef FFMPEG_TEST
#define RENDER_SUBTITLE  // 是否要將字幕加進video frame內
#endif


// 若有需要, 增加抽象介面.
class DLL_API Player
{
public:
    Player();
    virtual ~Player();

    Player( const Player& ) = delete;
    Player( Player&& ) = delete;

    Player& operator = ( const Player& ) = delete;
    Player& operator = ( Player&& ) = delete;

    virtual int     init();
    virtual void    play_QT();
    virtual int     end();

    //
    int     flush();
    void    stop();
    void    seek( int value, int old_value );
    void    set( DecodeSetting _setting );

    //
    bool    demux_need_wait();
    bool    is_set_input_file();
    int     decode( Decode *dc, AVPacket* pkt );
    int     init_demuxer();

    void    switch_subtitle( std::string path );
    void    switch_subtitle( int index );
    bool    is_embedded_subtitle();
    bool    is_file_subtitle();

    void    init_subtitle( AVFormatContext *fmt_ctx );
    void    handle_seek();    
    void    clear_setting();
    int64_t get_duration_time();

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
    void    set_output_jpg_root( std::string _root_path );
    void    set_output_audio_pcm_path( std::string _path );

    std::function<void(QImage)>     output_video_frame_func;
    std::function<void(AudioData)>  output_audio_pcm_func;
#else
    MediaInfo   get_media_info();   // use for output.
#endif
     
protected:
    bool    stop_flag   =   false;

    DecodeSetting&  get_setting();
    Demux*          get_demuxer();
    VideoDecode&    get_video_decoder();
    AudioDecode&    get_audio_decoder();
    SubDecode&      get_subtitle_decoder();

    std::function<void(Decode*)>    output_live_stream_func =   nullptr;

private:

    static constexpr int   MAX_QUEUE_SIZE  =   50;
    //static constexpr int   MAX_QUEUE_SIZE  =   10;

    DecodeSetting   setting;

    Demux           *demuxer    =   nullptr;
    VideoDecode     v_decoder;
    AudioDecode     a_decoder;
    SubDecode       s_decoder;

    // switch subtitle使用
    std::string     new_subtitle_path; 

    bool    switch_subtitle_flag    =   false;
    int     new_subtitle_index      =   0;

    // seek use
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
};




int     io_read_data( void *opaque, uint8_t *buf, int buf_size );



#ifdef FFMPEG_TEST
void    player_decode_example();
#endif



#endif