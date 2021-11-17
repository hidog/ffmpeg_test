#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H

#include "decode.h"


struct SwrContext;


// �յۭn��sample rate���S���\,�����|�A�ոլ�.

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
    //int     get_format_from_sample_fmt( const char **fmt, enum AVSampleFormat sample_fmt ); �����|�Ψ�

    int     init() override;
    int     end() override;

    AudioData   output_audio_data();



private:

    AVMediaType   type;
    
    SwrContext  *swr_ctx     =   nullptr; // use for chagne audio data to play.

};




#endif