#ifndef MAKER_IO_H
#define MAKER_IO_H


#include "maker.h"


class MakerIO : public Maker
{
public:

    // use for live stream
    void    release_encode_video_frame( AVFrame *vf );
    void    release_encode_audio_frame( AVFrame *af );


    void    work_live_stream();  // this will rename work()


private:
};




DLL_API int     io_write_data( void *opaque, uint8_t *buf, int buf_size );





#endif