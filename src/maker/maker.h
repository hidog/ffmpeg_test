#ifndef MAKER_H
#define MAKER_H


//#include "mux.h"
#include "tool.h"
//#include "audio_encode.h"
//#include "video_encode.h"
//#include "sub_encode.h"


class Mux;
class AudioEncode;
class VideoEncode;
class SubEncode;

struct EncodeSetting;
struct VideoEncodeSetting;
struct AudioEncodeSetting;
struct SubEncodeSetting;

/*
    未來如果要做 subtitle encode 的話
    do_subtitle_out, avcodec_encode_subtitle, 用這兩個關鍵字去搜尋
*/



class   Maker
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
    void    end();

private:

    Mux*    muxer  =   nullptr;

    AudioEncode*    a_encoder   =   nullptr;
    VideoEncode*    v_encoder   =   nullptr;
    SubEncode*      s_encoder   =   nullptr;

    EncodeSetting*  setting     =   nullptr;

};



DLL_API void    output_by_io();
int     io_write_data( void *opaque, uint8_t *buf, int buf_size );


#ifdef FFMPEG_TEST
void    maker_encode_example();
#endif



#endif