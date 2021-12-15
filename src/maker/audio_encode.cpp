#include "audio_encode.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "tool.h"


extern "C" {

#include <libavcodec/avcodec.h>

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>

}




/*******************************************************************************
AudioEncode::AudioEncode()
********************************************************************************/
AudioEncode::AudioEncode()
{}



/*******************************************************************************
AudioEncode::~AudioEncode()
********************************************************************************/
AudioEncode::~AudioEncode()
{}




/*******************************************************************************
AudioEncode::init()
********************************************************************************/
void    AudioEncode::init( AVCodecID code_id )
{
    int     ret;

    //
    codec   =   avcodec_find_encoder(code_id);
    if( codec == nullptr ) 
        MYLOG( LOG::ERROR, "codec = nullptr." );

    ctx     =   avcodec_alloc_context3(codec);
    if( ctx == nullptr ) 
        MYLOG( LOG::ERROR, "ctx = nullptr." );

    // some codec need set bit rate.
    //c->bit_rate = 64000;

    //
    ctx->sample_fmt     =   AV_SAMPLE_FMT_S16P;
    if( false == check_sample_fmt( codec, ctx->sample_fmt ) ) 
        MYLOG( LOG::ERROR, "fmt fail." );

    // init setting
    ctx->sample_rate    =   select_sample_rate(codec);
    ctx->channel_layout =   select_channel_layout(codec);
    ctx->channels       =   av_get_channel_layout_nb_channels(ctx->channel_layout);

    // open
    ret     =   avcodec_open2( ctx, codec, nullptr );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "open fail." );

    //
    pkt     =   av_packet_alloc();
    if( nullptr == pkt )
        MYLOG( LOG::ERROR, "pkt alloc fail" );

    //
    frame   =   av_frame_alloc();
    if( nullptr == frame )
        MYLOG( LOG::ERROR, "frame alloc fail" );

    // set param to frame.
    frame->nb_samples       =   ctx->frame_size;
    frame->format           =   ctx->sample_fmt;
    frame->channel_layout   =   ctx->channel_layout;

    // allocate the data buffers
    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "alloc buffer fail" );
}





/*******************************************************************************
AudioEncode::list_sample_format()
********************************************************************************/
void    AudioEncode::list_sample_format( AVCodecID code_id )
{
    AVCodec*                codec   =   avcodec_find_encoder(code_id);
    const AVSampleFormat*   p_fmt   =   codec->sample_fmts;

    while( *p_fmt != AV_SAMPLE_FMT_NONE )
    {
        printf( "%s support %s\n", avcodec_get_name(code_id), av_get_sample_fmt_name(*p_fmt) );
        p_fmt++;
    }
}




/*******************************************************************************
AudioEncode::list_sample_format()
********************************************************************************/
void    AudioEncode::list_sample_rate( AVCodecID code_id )
{
    AVCodec*     codec   =   avcodec_find_encoder(code_id);
    const int*   p_smp   =   codec->supported_samplerates;

    while( *p_smp != 0 )
    {
        printf( "%s support sample rate %d\n", avcodec_get_name(code_id), *p_smp );
        p_smp++;
    }
}






/*******************************************************************************
AudioEncode::select_sample_rate()
********************************************************************************/
int     AudioEncode::select_sample_rate( AVCodec *codec )
{
    const int   *p_smr  =   nullptr;
    int     best_samplerate     =   0;

    if( codec->supported_samplerates == nullptr )
        return 44100;

    p_smr   =   codec->supported_samplerates;
    while( *p_smr != 0 ) 
    {
        if( 0 == best_samplerate || abs(44100 - *p_smr) < abs(44100 - best_samplerate))
            best_samplerate     =   *p_smr;
        p_smr++;
    }

    return best_samplerate;
}






/*******************************************************************************
AudioEncode::check_sample_fmt()
********************************************************************************/
bool    AudioEncode::check_sample_fmt( AVCodec *codec, AVSampleFormat sample_fmt )
{
    const AVSampleFormat    *p_fmt  =   codec->sample_fmts;

    while( *p_fmt != AV_SAMPLE_FMT_NONE )
    {
        if( *p_fmt == sample_fmt )
            return  true;
        p_fmt++;
    }
    return  false;
}




/*******************************************************************************
AudioEncode::list_channel_layout()
********************************************************************************/
void    AudioEncode::list_channel_layout( AVCodecID code_id )
{
    AVCodec*     codec   =   avcodec_find_encoder(code_id);

    const uint64_t  *p_cl   =   nullptr;
    uint64_t    best_ch_layout      =   0;
    int         best_nb_channels    =   0;

    p_cl    =   codec->channel_layouts;
    if( p_cl == nullptr )
    {
        printf("can not list.\n");
        return;
    }

    while( *p_cl != 0 ) 
    {
        int nb_channels = av_get_channel_layout_nb_channels(*p_cl);
        printf( "%s support channel layout %llu %d\n", avcodec_get_name(code_id), *p_cl, nb_channels );
        p_cl++;
    }
}







/*******************************************************************************
AudioEncode::select_channel_layout()
********************************************************************************/
int     AudioEncode::select_channel_layout( AVCodec *codec )
{
    const uint64_t  *p_cl   =   nullptr;
    uint64_t    best_ch_layout      =   0;
    int         best_nb_channels    =   0;

    if( codec->channel_layouts == nullptr )
        return AV_CH_LAYOUT_STEREO;

    p_cl    =   codec->channel_layouts;
    while( *p_cl != 0 ) 
    {
        int     nb_channels     =   av_get_channel_layout_nb_channels(*p_cl);

        if( nb_channels > best_nb_channels ) 
        {
            best_ch_layout      =   *p_cl;
            best_nb_channels    =   nb_channels;
        }
        p_cl++;
    }

    return best_ch_layout;
}







/*******************************************************************************
AudioEncode::encode()
********************************************************************************/
void    AudioEncode::encode( AVFrame *frame )
{
    int     ret;

    ret     =   avcodec_send_frame( ctx, frame );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "send fail." );

    while( ret >= 0 ) 
    {
        ret     =   avcodec_receive_packet( ctx, pkt );
        if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
            return;
        else if (ret < 0) 
            MYLOG( LOG::ERROR, "recv fail." );

        printf("write data %d...\n", pkt->size );
        fwrite( pkt->data, 1, pkt->size, output );
        av_packet_unref(pkt);
    }
}





/*******************************************************************************
AudioEncode::end()
********************************************************************************/
void    AudioEncode::end()
{
    av_frame_free( &frame );
    av_packet_free( &pkt );
    avcodec_free_context( &ctx );
}



/*******************************************************************************
AudioEncode::work()
********************************************************************************/
void     AudioEncode::work( AVCodecID code_id )
{
    if( code_id == AV_CODEC_ID_MP3 )
        output  =   fopen( "H:\\test.mp3", "wb+" );

    FILE    *fp     =   fopen( "H:\\test.pcm", "rb" );
    int     i;
    int16_t     intens[2];


    while( 0 == feof(fp) )
    {
        for( i = 0; i < frame->nb_samples; i++ )
        {
            fread( intens, sizeof(int16_t)*2, 1, fp );

            // 多聲道這邊需要處理
            *((int16_t*)(frame->data[0]) + i) = intens[0];
            *((int16_t*)(frame->data[1]) + i) = intens[1];
        }

        encode( frame );
    }

    /* flush the encoder */
    encode( NULL );

    fclose(output);
    fclose(fp);
}
