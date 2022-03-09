#ifndef DECODE_MANAGER_H
#define DECODE_MANAGER_H

#include <map>
#include <functional>
#include <string>

#include "play_def.h"


class Decode;
class VideoDecode;
class AudioDecode;
class SubDecode;
struct AVFormatContext;



constexpr int file_subtitle_index   =   1000;    // subtitle_map[file_subtitle_index] 用來處理外部ass檔案的字幕檔



class DecodeManager
{
public:
    DecodeManager();
    ~DecodeManager();

    DecodeManager( const DecodeManager& ) = delete;
    DecodeManager( DecodeManager&& ) = delete;

    DecodeManager& operator = ( const DecodeManager& ) = delete;
    DecodeManager& operator = ( DecodeManager&& ) = delete;

    int     open_decoders( AVFormatContext* fmt_ctx );
    void    init_decoders();
    void    flush_decoders();
    void    flush_decoders_for_seek();

    Decode*     get_decoder( int stream_index );

    bool    exist_video_stream();
    bool    exist_audio_stream();
    bool    exist_subtitle_stream();
    bool    is_video_attachd();

    int     get_current_video_index();
    int     get_current_audio_index();

    VideoDecode*    get_current_video_decoder();
    AudioDecode*    get_current_audio_decoder();
    SubDecode*      get_current_subtitle_decoder();

    void    set_subtitle_file( std::string path );
    void    set_sub_src_type( SubSourceType type );
    void    init_subtitle( AVFormatContext *fmt_ctx, DecodeSetting setting );

#ifdef FFMPEG_TEST
    void    set_output_jpg_path( std::string _root_path );
    void    set_output_audio_pcm_path( std::string _path );
#endif

private:

    std::map<int,VideoDecode*>  video_map;
    std::map<int,AudioDecode*>  audio_map;
    std::map<int,SubDecode*>    subtitle_map;

    int     current_video_index     =   -1;
    int     current_audio_index     =   -1;
    int     current_subtitle_index  =   -1;

    SubSourceType   sub_src_type    =   SubSourceType::NONE;

};



#endif