#ifndef PLAYER_H
#define PLAYER_H

#include "tool.h"

#ifdef USE_MT
#include <queue>
#include <thread>
#include <mutex>
#endif

#include <functional>
#include <memory>

#include "demux.h"
#include "decode.h"
#include "decode_manager.h"



/*#include "audio_decode.h"
#include "sub_decode.h"
#include "video_decode_nv.h"
#include "video_decode_hw.h"*/

#include <QImage>


struct AVPacket;



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
    bool    exist_video_stream();

    void    switch_subtitle( std::string path );
    void    switch_subtitle( int index );
    bool    is_embedded_subtitle();
    bool    is_file_subtitle();

    //void    init_subtitle( AVFormatContext *fmt_ctx );
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
    void    set_output_jpg_path( std::string _path );
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

    /*VideoDecode&    get_video_decoder();
    AudioDecode&    get_audio_decoder();
    SubDecode&      get_subtitle_decoder();*/

    std::function<void(Decode*)>    output_live_stream_func =   nullptr;

private:

    static constexpr int   MAX_QUEUE_SIZE  =   200;   // queue 太小有機會出問題
    //static constexpr int   MAX_QUEUE_SIZE  =   10;

    DecodeSetting   setting;

    Demux           *demuxer        =   nullptr;
    std::unique_ptr<DecodeManager>  decode_manager;

    //VideoDecode     v_decoder;
    //VideoDecodeNV   v_decoder;
    //VideoDecodeHW   v_decoder;
    //AudioDecode     a_decoder;
    //SubDecode       s_decoder;

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


#ifdef FFMPEG_TEST
void    player_decode_example();
#endif



#endif