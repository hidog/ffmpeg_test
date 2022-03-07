#ifndef DECODE_MANAGER_H
#define DECODE_MANAGER_H

#include <map>
#include <functional>
#include <string>


class Decode;
class VideoDecode;
class AudioDecode;
struct AVFormatContext;




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

    bool    exist_video_stream();
    bool    exist_audio_stream();

    VideoDecode*    get_current_video_decoder();
    AudioDecode*    get_current_audio_decoder();

#ifdef FFMPEG_TEST
    void    set_output_jpg_path( std::string _root_path );
    void    set_output_audio_pcm_path( std::string _path );
#endif

private:

    std::map<int,Decode*>   video_map;
    std::map<int,Decode*>   audio_map;
    std::map<int,Decode*>   subtitle_map;

    int     current_video_index     =   -1;
    int     current_audio_index     =   -1;
    int     current_subtitle_index  =   -1;

};



#endif