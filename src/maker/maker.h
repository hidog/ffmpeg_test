#ifndef MAKER_H
#define MAKER_H


#include "mux.h"
#include "audio_encode.h"
#include "video_encode.h"
#include "sub_encode.h"


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

    void    init( EncodeSetting _setting, VideoEncodeSetting v_setting, AudioEncodeSetting a_setting, SubEncodeSetting s_setting );
    void    init_muxer();

    void    work();
    void    work_with_subtitle();
    void    work_without_subtitle();
    void    end();

private:

    Mux*    muxer  =   nullptr;

    AudioEncode     a_encoder;
    VideoEncode     v_encoder;
    SubEncode       s_encoder;

    EncodeSetting   setting;

};




int     io_write_data( void *opaque, uint8_t *buf, int buf_size );



void    maker_encode_example();



#endif