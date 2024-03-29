#ifndef AUDIO_ENCODE_H
#define AUDIO_ENCODE_H


#include <stdint.h>
#include "encode.h"
#include "maker_def.h"

/* 
https://blog.csdn.net/wanggao_1990/article/details/115725163

NOTE: 如果要改sample rate 48000 -> 44100
可以參考 resampling_audio 官方範例.
利用 swr_convert 可做轉換, 需要的 buffer 可以用 av_samples_alloc_array_and_samples 取得. 
這邊考慮其他因素, 直接使用 frame 跟 pointer 處理.

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

    // 底下兩個函數目前不能動, 需要修改.
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
        可以使用 av_samples_alloc_array_and_samples 取得資料
        但考慮 pcm 可能是其他 framework 傳入, 改用 point 處理.
        目前傳入格式通常是 S16.
    */
    int16_t*    pcm[2]      =   { nullptr, nullptr };   
    int         pcm_size    =   0;

    std::string     load_pcm_path   =   "J:\\test.pcm";
#endif
};




#endif