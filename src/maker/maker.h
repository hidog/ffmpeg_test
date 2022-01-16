#ifndef MAKER_H
#define MAKER_H



#include "tool.h"
#include "../player/play_def.h"



class Mux;
class AudioEncode;
class VideoEncode;
class SubEncode;
class Encode;

struct EncodeSetting;
struct VideoEncodeSetting;
struct AudioEncodeSetting;
struct SubEncodeSetting;
struct AVFrame;
struct AVRational;






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
    void    end();

    bool    is_connect();
    void    flush_encoder( Encode* enc, AVRational* st_tb );

    //EncodeOrder order_pts_func();
    void order_pts_func();



private:

    Mux*    muxer  =   nullptr;

    AudioEncode*    a_encoder   =   nullptr;
    VideoEncode*    v_encoder   =   nullptr;
    SubEncode*      s_encoder   =   nullptr;

    EncodeSetting*  setting     =   nullptr;

};



DLL_API void    output_by_io( MediaInfo media_info, std::string _port, Maker& maker );


#ifdef FFMPEG_TEST
void    maker_encode_example();
#endif



#endif