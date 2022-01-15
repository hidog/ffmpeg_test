#ifndef MAKER_H
#define MAKER_H



#include "tool.h"
#include "../player/play_def.h"



class Mux;
class AudioEncode;
class VideoEncode;
class SubEncode;

struct EncodeSetting;
struct VideoEncodeSetting;
struct AudioEncodeSetting;
struct SubEncodeSetting;
struct AVFrame;


// push frame to queue, and use for encode.
DLL_API void    add_audio_frame( AVFrame* af );
DLL_API void    add_video_frame( AVFrame* vf );

DLL_API AVFrame*    get_audio_frame();
DLL_API AVFrame*    get_video_frame();




class DLL_API Maker
{
public:

    Maker();
    ~Maker();

    Maker( const Maker& ) = delete;
    Maker( Maker&& ) = delete;

    Maker& operator = ( const Maker& ) = delete;
    Maker& operator = ( Maker&& ) = delete;

    void    init( EncodeSetting* _setting, VideoEncodeSetting* v_setting, AudioEncodeSetting* a_setting, SubEncodeSetting* s_setting );
    void    init_muxer();

    void    work();
    void    work_with_subtitle();
    void    work_without_subtitle();
    void    work_live_stream();
    void    end();

    bool    is_connect();

private:

    Mux*    muxer  =   nullptr;

    AudioEncode*    a_encoder   =   nullptr;
    VideoEncode*    v_encoder   =   nullptr;
    SubEncode*      s_encoder   =   nullptr;

    EncodeSetting*  setting     =   nullptr;

};



DLL_API void    output_by_io( MediaInfo media_info, std::string _port, Maker& maker );
DLL_API int     io_write_data( void *opaque, uint8_t *buf, int buf_size );


#ifdef FFMPEG_TEST
void    maker_encode_example();
#endif



#endif