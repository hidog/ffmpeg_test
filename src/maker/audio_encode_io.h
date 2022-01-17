#ifndef AUDIO_ENCODE_IO_H
#define AUDIO_ENCODE_IO_H


#include "audio_encode.h"


class AudioEncodeIO : public AudioEncode
{
public:
    AudioEncodeIO();
    ~AudioEncodeIO();

    void    init( int st_idx, AudioEncodeSetting setting, bool need_global_header ) override;

    void        next_frame() override;



private:


};



#endif