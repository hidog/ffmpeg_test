#ifndef MAKER_IO_H
#define MAKER_IO_H


#include "maker.h"


class InputOutput;



class MakerIO : public Maker
{
public:

    MakerIO();
    ~MakerIO();

    MakerIO( const MakerIO& ) = delete;
    MakerIO( MakerIO&& ) = delete;

    MakerIO& operator = ( const MakerIO& ) = delete;
    MakerIO& operator = ( MakerIO&& ) = delete;


    void    init( EncodeSetting _setting, VideoEncodeSetting v_setting, AudioEncodeSetting a_setting );
    void    init_IO();

    bool    is_connect() override;


    // use for live stream
    void    release_encode_video_frame( AVFrame *vf );
    void    release_encode_audio_frame( AVFrame *af );


    void    work();  // this will rename work()


private:


    InputOutput*    IO      =   nullptr;


};




DLL_API int     io_write_data( void *opaque, uint8_t *buf, int buf_size );





#endif