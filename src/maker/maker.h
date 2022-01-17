#ifndef MAKER_H
#define MAKER_H



#include "tool.h"
#include "maker_interface.h"
#include "audio_encode.h"
#include "video_encode.h"
#include "sub_encode.h"

#include "../player/play_def.h"



class Mux;

/*class AudioEncode;
class VideoEncode;
class SubEncode;
class Encode;*/

struct EncodeSetting;
struct VideoEncodeSetting;
struct AudioEncodeSetting;
struct SubEncodeSetting;
struct AVFrame;
struct AVRational;






class Maker : public MakerInterface
{
public:

    Maker();
    virtual ~Maker();

    Maker( const Maker& ) = delete;
    Maker( Maker&& ) = delete;

    Maker& operator = ( const Maker& ) = delete;
    Maker& operator = ( Maker&& ) = delete;

    void    init( EncodeSetting _setting, VideoEncodeSetting v_setting, AudioEncodeSetting a_setting, SubEncodeSetting s_setting );

    void    work() override;
    void    end() override;
    bool    is_connect() override;

    void    work_with_subtitle();
    void    work_without_subtitle();
    void    flush_encoder( Encode* enc );


protected:

    Mux*    muxer  =   nullptr;

    AudioEncode     a_encoder;
    VideoEncode     v_encoder;
    SubEncode       s_encoder;

    EncodeSetting   setting;

};



#ifdef FFMPEG_TEST
void    maker_encode_example();
#endif



#endif