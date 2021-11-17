#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H

#include "decode.h"


struct SwrContext;


// 試著要轉sample rate但沒成功,有機會再試試看.

class DLL_API AudioDecode : public Decode
{
public:
    AudioDecode();
    ~AudioDecode();

    AudioDecode( const AudioDecode& ) = delete;
    AudioDecode( AudioDecode&& ) = delete;

    AudioDecode& operator = ( const AudioDecode& ) = delete;
    AudioDecode& operator = ( AudioDecode&& ) = delete;

    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx ) override;
    int     output_frame() override;
    void    print_finish_message() override;
    int     get_format_from_sample_fmt( const char **fmt, enum AVSampleFormat sample_fmt );

    int     init() override;
    int     end() override;

    AudioData   output_PCM();


    myAVMediaType   get_type() override { return type; } 


private:

    const myAVMediaType   type    =   myAVMediaType::AVMEDIA_TYPE_AUDIO; 
    
    SwrContext  *swr_ctx     =   nullptr; // use for chagne audio data to play.

};




#endif