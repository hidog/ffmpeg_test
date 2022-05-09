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

    void    release();

    int     open_decoders( AVFormatContext* fmt_ctx );
    void    init_decoders();

    void    flush_decoders_for_seek();
    void    flush_all_sub_stream();
    void    flush_video();
    void    flush_audio();

    Decode*     get_decoder( int stream_index );

    bool    index_is_available( int index );
    bool    exist_video_stream();
    bool    exist_audio_stream();
    bool    exist_subtitle_stream();
    bool    is_video_attachd();

    int     get_current_video_index();
    int     get_current_audio_index();
    int     get_current_subtitle_index();

    bool    is_current_audio_index( int index );

    VideoDecode*    get_current_video_decoder();
    AudioDecode*    get_current_audio_decoder();
    SubDecode*      get_current_subtitle_decoder();

    void    set_subtitle_file( std::string path );
    void    set_sub_src_type( SubSourceType type );
    void    init_subtitle( AVFormatContext *fmt_ctx, DecodeSetting setting );
    void    switch_subtltle( std::string path );
    void    switch_subtltle( int index );
    void    set_subfile( std::string path );

    SubSourceType   get_sub_src_type();
    std::pair<std::string,std::string>  get_subtitle_param( AVFormatContext* fmt_ctx, std::string src_file, SubData sd );
    std::vector<std::string>            get_embedded_subtitle_list();

#ifdef FFMPEG_TEST
    void    set_output_jpg_path( std::string _root_path );
    void    set_output_audio_pcm_path( std::string _path );
    void    flush_decoders();
#endif

private:

    std::map<int,VideoDecode*>  video_map;
    std::map<int,AudioDecode*>  audio_map;
    std::map<int,SubDecode*>    subtitle_map;   // 實際上都用同一個subtitle decode負責render. 會造成多佔記憶體

    int     current_video_index     =   -1;
    int     current_audio_index     =   -1;
    int     current_subtitle_index  =   -1;

    int     subtitle_index   =   0;
    SubSourceType   sub_src_type    =   SubSourceType::NONE;
    std::string     subtitle_file;
    std::string     subtitle_args;
};



#endif