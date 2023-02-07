#ifndef AUDIO_ENCODE_H
#define AUDIO_ENCODE_H


#include <stdint.h>
#include "encode.h"
#include "maker_def.h"

/* 
https://blog.csdn.net/wanggao_1990/article/details/115725163

NOTE: �p�G�n��sample rate 48000 -> 44100
�i�H�Ѧ� resampling_audio �x��d��.
�Q�� swr_convert �i���ഫ, �ݭn�� buffer �i�H�� av_samples_alloc_array_and_samples ���o. 
�o��Ҽ{��L�]��, �����ϥ� frame �� pointer �B�z.

*/


enum AVCodecID;
enum AVSampleFormat;

struct AVCodec;
struct SwrContext;



class AudioEncode : public Encode
{
public:
    AudioEncode();
    ~AudioEncode();

    AudioEncode( const AudioEncode& ) = delete;
    AudioEncode( AudioEncode&& ) = delete;

    AudioEncode& operator = ( const AudioEncode& ) = delete;
    AudioEncode& operator = ( const AudioEncode&& ) = delete;

    void    init( int st_idx, AudioEncodeSetting setting, bool need_global_header );
    void    list_sample_format( AVCodecID code_id );
    void    list_sample_rate( AVCodecID code_id );
    void    list_channel_layout( AVCodecID code_id );

    void    end() override;
    void    next_frame() override;
    void    unref_data() override;
    int64_t get_pts() override;

#ifdef FFMPEG_TEST
    void    init_swr( AudioEncodeSetting setting );
    void    get_frame_from_pcm_file();
    void    get_frame_from_file_test();

    // ���U��Ө�ƥثe�����, �ݭn�ק�.
    void    encode_test();
    void    work_test();
#endif

private:

    bool    check_sample_fmt( AVCodec *codec, AVSampleFormat sample_fmt );
    int     select_sample_rate( AVCodec *codec );
    int     select_channel_layout( const AVCodec *codec );

#ifdef FFMPEG_TEST
    char*   adts_head( int packetlen );

    SwrContext  *swr_ctx    =   nullptr;

    /*
        �i�H�ϥ� av_samples_alloc_array_and_samples ���o���
        ���Ҽ{ pcm �i��O��L framework �ǤJ, ��� point �B�z.
        �ثe�ǤJ�榡�q�`�O S16.
    */
    int16_t*    pcm[2]      =   { nullptr, nullptr };   
    int         pcm_size    =   0;

    std::string     load_pcm_path   =   "J:\\test.pcm";
#endif
};




#endif