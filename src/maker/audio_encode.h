#ifndef AUDIO_ENCODE_H
#define MAKAUDIO_ENCODE_HER_H

#include <stdio.h>


enum AVCodecID;
enum AVSampleFormat;
struct AVCodec;

struct AVCodecContext;
struct AVFrame;
struct AVPacket;



class AudioEncode
{
public:
    AudioEncode();
    ~AudioEncode();

    AudioEncode( const AudioEncode& ) = delete;
    AudioEncode( AudioEncode&& ) = delete;

    AudioEncode& operator = ( const AudioEncode& ) = delete;
    AudioEncode& operator = ( const AudioEncode&& ) = delete;

    void    init( AVCodecID code_id );
    void    end();

    void    list_sample_format( AVCodecID code_id );
    void    list_sample_rate( AVCodecID code_id );
    void    list_channel_layout( AVCodecID code_id );

    void    encode( AVFrame *frame );
    void    work( AVCodecID code_id );

private:

    bool    check_sample_fmt( AVCodec *codec, AVSampleFormat sample_fmt );
    int     select_sample_rate( AVCodec *codec );
    int     select_channel_layout( AVCodec *codec );

    AVCodec         *codec  =   nullptr;
    AVCodecContext  *ctx    =   nullptr;
    AVFrame         *frame  =   nullptr;
    AVPacket        *pkt    =   nullptr;

    FILE    *output;
};




#endif