#ifndef AUDIO_ENCODE_H
#define AUDIO_ENCODE_H

#include <stdio.h>
#include <stdint.h>

#include "encode.h"
#include "maker_def.h"

// https://blog.csdn.net/wanggao_1990/article/details/115725163


enum AVCodecID;
enum AVSampleFormat;

struct AVCodec;
struct AVFrame;
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
    void    end() override;
    void    init_swr( AudioEncodeSetting setting );

    void    list_sample_format( AVCodecID code_id );
    void    list_sample_rate( AVCodecID code_id );
    void    list_channel_layout( AVCodecID code_id );

    int64_t     get_pts() override;
    AVFrame*    get_frame() override;
    int         send_frame() override;

    // for test, run without mux.
    // 目前不能動, 需要修改.
    void    encode_test();
    void    work_test();

private:

    bool    check_sample_fmt( AVCodec *codec, AVSampleFormat sample_fmt );
    int     select_sample_rate( AVCodec *codec );
    int     select_channel_layout( AVCodec *codec );

    char*   adts_head( int packetlen );

    SwrContext*     swr_ctx     =   nullptr;

    uint8_t     **audio_data        =   { nullptr };
    int         audio_linesize      =   0;
    int         audio_bufsize       =   0;


    //AVFrame*    src_frame;
    uint8_t     **src_data        =   { nullptr };
    int         src_linesize      =   0;
    int         src_bufsize       =   0;
};




#endif