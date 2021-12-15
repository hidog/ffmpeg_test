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

        fwrite( adts_gen(pkt->size), 7, 1, output );

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
    if( code_id == AV_CODEC_ID_AAC )   
        ctx->bit_rate   =   64000;
    else if( code_id == AV_CODEC_ID_MP2 )
        ctx->bit_rate   =   64000; // 沒設置也能播放 

    //
    if( code_id == AV_CODEC_ID_MP3 )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_S16P;
    else if( code_id == AV_CODEC_ID_AAC )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_FLTP;
    else if( code_id == AV_CODEC_ID_MP2 )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_S16;

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
AudioEncode::adts_gen()
********************************************************************************/
char* AudioEncode::adts_gen( const int packetlen )
{
    static char packet[7];
    
    int profile = 2;
    int freqidx = 3;
    int chancfg = 2; 

    packet[0] = 0xFF;
    packet[1] = 0xF1;
    packet[2] = ((profile - 1) << 6) + (freqidx << 2) + (chancfg >> 2);
    packet[3] = ((chancfg % 3) << 6) + (packetlen >> 11);
    packet[4] = (packetlen & 0x7FF ) >> 3;
    packet[5] = ((packetlen & 7 ) << 5) + 0x1F;
    packet[6] = 0xFC;

    return packet;
}



/*******************************************************************************
AudioEncode::work()
********************************************************************************/
void     AudioEncode::work( AVCodecID code_id )
{
    if( code_id == AV_CODEC_ID_MP3 )
        output  =   fopen( "H:\\test.mp3", "wb+" );
    else if( code_id == AV_CODEC_ID_AAC )
        output  =   fopen( "H:\\test.aac", "wb+" );
    else if( code_id == AV_CODEC_ID_MP2 )
        output  =   fopen( "H:\\test.mp2", "wb+" );

    FILE    *fp     =   fopen( "H:\\test.pcm", "rb" );
    int     i;
    int16_t     intens[2];

    if( code_id == AV_CODEC_ID_AAC )
    {
        while( 0 == feof(fp) )
        {
            for( i = 0; i < frame->nb_samples; i++ )
            {
                fread( intens, 2, sizeof(int16_t), fp );

                // 多聲道這邊需要處理
                *((float*)(frame->data[0]) + i)   =   1.0 * intens[0] / 32768.0;
                *((float*)(frame->data[1]) + i)   =   1.0 * intens[1] / 32768.0;
            }
            encode( frame );
        }
    }
    else if( code_id == AV_CODEC_ID_MP3 )
    {
        while( 0 == feof(fp) )
        {
            for( i = 0; i < frame->nb_samples; i++ )
            {
                fread( intens, 2, sizeof(int16_t), fp );
                *((int16_t*)(frame->data[0]) + i)   =   intens[0];
                *((int16_t*)(frame->data[1]) + i)   =   intens[1];
            }
            encode( frame );
        }
    }
    else if( code_id == AV_CODEC_ID_MP2 )
    {
        while( 0 == feof(fp) )
        {
            for( i = 0; i < frame->nb_samples; i++ )
            {
                fread( intens, sizeof(int16_t)*2, 1, fp );
                *((int16_t*)(frame->data[0]) + 2*i     )   =   intens[0];
                *((int16_t*)(frame->data[0]) + 2*i + 1 )   =   intens[1];
            }
            encode( frame );
        }
    }

    /* flush the encoder */
    encode( NULL );

    fclose(output);
    fclose(fp);
}
