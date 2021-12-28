#ifndef MAKER_H
#define MAKER_H

#include "audio_encode.h"
#include "video_encode.h"
#include "mux.h"


/*
    ���Ӧp�G�n�� subtitle encode ����
    do_subtitle_out, avcodec_encode_subtitle, �γo�������r�h�j�M
*/


class Maker
{
public:

    Maker();
    ~Maker();

    Maker( const Maker& ) = delete;
    Maker( Maker&& ) = delete;

    Maker& operator = ( const Maker& ) = delete;
    Maker& operator = ( Maker&& ) = delete;

    void    init( EncodeSetting setting, VideoEncodeSetting v_setting, AudioEncodeSetting a_setting );
    void    work();
    void    end();

private:

    AudioEncode a_encoder;
    VideoEncode v_encoder;
    Mux muxer;

};



#endif