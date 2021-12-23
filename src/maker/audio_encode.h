#ifndef AUDIO_ENCODE_H
#define AUDIO_ENCODE_H

#include <stdio.h>
#include <stdint.h>

#include "encode.h"

// https://blog.csdn.net/wanggao_1990/article/details/115725163


enum AVCodecID;
enum AVSampleFormat;
struct AVCodec;

struct AVFrame;



struct AudioEncodeSetting
{
    AVCodecID   code_id;
    int64_t     bit_rate;
    int         sample_rate;
};



class AudioEncode : public Encode
{
public:
    AudioEncode();
    ~AudioEncode();

    AudioEncode( const AudioEncode& ) = delete;
    AudioEncode( AudioEncode&& ) = delete;

    AudioEncode& operator = ( const AudioEncode& ) = delete;
    AudioEncode& operator = ( const AudioEncode&& ) = delete;

    void    init( int st_idx, AudioEncodeSetting a_setting );
    void    end();

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

    // 需要加上 sws 用資料.

    bool    check_sample_fmt( AVCodec *codec, AVSampleFormat sample_fmt );
    int     select_sample_rate( AVCodec *codec );
    int     select_channel_layout( AVCodec *codec );

    char*   adts_head( int packetlen );

};




#endif