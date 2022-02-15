#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H

#include "decode.h"


struct  SwrContext;
struct  AVCodec;
struct  AVCodecContext;
enum    AVSampleFormat;


/*
https://www.jianshu.com/p/fd43c1c82945   PCM
https://www.jianshu.com/p/bf5e54f553a4   ���s�ļ�
*/

// �n�ഫ sample rate �i�H�Ѧ� ffmpeg �x��d�Ҫ� resample


class DLL_API AudioDecode : public Decode
{
public:
    AudioDecode();
    ~AudioDecode();

    AudioDecode( const AudioDecode& ) = delete;
    AudioDecode( AudioDecode&& ) = delete;

    AudioDecode& operator = ( const AudioDecode& ) = delete;
    AudioDecode& operator = ( AudioDecode&& ) = delete;
    
    int     open_codec_context( AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;
    
    int     init() override;
    int     end() override;

    int     get_audio_nb_sample();
    int     get_audio_channel();
    int     get_audio_channel_layout();
    int     get_audio_sample_rate();
    int     get_audio_sample_format();

    void        output_audio_frame_info();    
    AudioData   output_audio_data();
    int64_t     get_timestamp();

#ifdef FFMPEG_TEST
    int     output_pcm();
    void    set_output_audio_pcm_path( std::string _path );
#endif

    int     audio_info(); // �ثe�L�@�� ���ӦҼ{����

private:

    int         sample_rate     =   0;
    uint64_t    channel_layout  =   0;  // �h�n�D���ɭԬ�s�@�U

    AVSampleFormat  sample_fmt;
    SwrContext      *swr_ctx    =   nullptr;   // use for chagne audio data to play.

#ifdef FFMPEG_TEST
    std::string     output_pcm_path =   "H:\\test.pcm";
#endif

};




#endif