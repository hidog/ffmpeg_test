#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H

#include "decode.h"


struct SwrContext;
struct AVCodec;
struct AVCodecContext;
enum AVSampleFormat;


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

    //
    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;

    //
    int     init() override;
    int     end() override;

    //
    void    output_audio_frame_info();

    //
    AudioData   output_audio_data();

private:

    int         sample_rate     =   0;
    uint64_t    channel_layout  =   0;  // �h�n�D���ɭԬ�s�@�U

    AVSampleFormat  sample_fmt;

    AVMediaType     type;    
    SwrContext      *swr_ctx    =   nullptr; // use for chagne audio data to play.

};




#endif