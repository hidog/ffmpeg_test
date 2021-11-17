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
    //int     get_format_from_sample_fmt( const char **fmt, enum AVSampleFormat sample_fmt ); 有機會用到

    int     init() override;
    int     end() override;

    AudioData   output_audio_data();



private:

    AVMediaType   type;
    
    SwrContext  *swr_ctx     =   nullptr; // use for chagne audio data to play.

};




#endif