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
    :   Encode()
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

    if( p_smp == nullptr )
    {
        printf("not defined.\n");
        return;
    }

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
AudioEncode::end()
********************************************************************************/
void    AudioEncode::end()
{
    Encode::end();

    av_frame_free( &frame );
    av_packet_free( &pkt );
    avcodec_free_context( &ctx );
}






/*******************************************************************************
AudioEncode::init()
********************************************************************************/
void    AudioEncode::init( int st_idx, AudioEncodeSetting a_setting )
{
    AVCodecID   code_id     =   a_setting.code_id;
    int         ret;

    Encode::init( st_idx, code_id );

    // some codec need set bit rate.
    // 驗證一下這件事情. 部分 codec 會自動產生預設 bit rate.
    if( a_setting.code_id != AV_CODEC_ID_FLAC )
        ctx->bit_rate   =   a_setting.bit_rate;
    
    // format可更改,但支援度跟codec有關.
    if( code_id == AV_CODEC_ID_MP3 )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_S16P;
    else if( code_id == AV_CODEC_ID_AAC || code_id == AV_CODEC_ID_AC3 )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_FLTP;
    else if( code_id == AV_CODEC_ID_MP2 || code_id == AV_CODEC_ID_FLAC )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_S16;
    else    
        MYLOG( LOG::ERROR, "un handle codec" );    

    if( false == check_sample_fmt( codec, ctx->sample_fmt ) ) 
        MYLOG( LOG::ERROR, "fmt fail." );

    // init setting
    ctx->sample_rate    =   a_setting.sample_rate; 
    ctx->channel_layout =   AV_CH_LAYOUT_STEREO;
    ctx->channels       =   av_get_channel_layout_nb_channels(ctx->channel_layout);

    // open ctx.
    ret     =   avcodec_open2( ctx, codec, nullptr );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "open fail." );

    //
    if( ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE )
        MYLOG( LOG::WARN, "need set to 10000" );

    // set param to frame.
    frame->nb_samples       =   ctx->frame_size;
    frame->format           =   ctx->sample_fmt;
    frame->channel_layout   =   ctx->channel_layout;
    frame->sample_rate      =   ctx->sample_rate;
    frame->pts              =   -frame->nb_samples;

    // allocate the data buffers
    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "alloc buffer fail" );

    ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}






/*******************************************************************************
AudioEncode::encode()

目前不能動
********************************************************************************/
void    AudioEncode::encode_test()
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

        //if( code_id == AV_CODEC_ID_AAC )
            //fwrite( adts_head(pkt->size), 1, 7, output );

        //fwrite( pkt->data, 1, pkt->size, output );
        av_packet_unref(pkt);
    }
}








/*******************************************************************************
AudioEncode::adts_gen()

aac 格式, 每個 frame 要加上head, 才能獨立撥放
********************************************************************************/
char* AudioEncode::adts_head( int packetlen )
{
    packetlen   +=  7;

    static char packet[7];
    
    int profile = 2;
    int freqidx = 3; // 48000 hz. 改sample rate要跟著動這個參數
    int chancfg = 2; 

    packet[0] = 0xFF;
    packet[1] = 0xF1;
    packet[2] = ((profile - 1) << 6) + (freqidx << 2) + (chancfg >> 2);
    packet[3] = ((chancfg & 3) << 6) + ( packetlen >> 11);
    packet[4] = (packetlen & 0x7FF ) >> 3;
    packet[5] = ((packetlen & 7 ) << 5) + 0x1F;
    packet[6] = 0xFC;

    return packet;
}




/*******************************************************************************
AudioEncode::work_test()

目前不能動.
********************************************************************************/
void     AudioEncode::work_test()
{
#if 0
    int     ret;

    if( code_id == AV_CODEC_ID_MP3 )
        output  =   fopen( "H:\\test.mp3", "wb+" );
    else if( code_id == AV_CODEC_ID_AAC )
        output  =   fopen( "H:\\test.aac", "wb+" );
    else if( code_id == AV_CODEC_ID_MP2 )
        output  =   fopen( "H:\\test.mp2", "wb+" );
    else if( code_id == AV_CODEC_ID_AC3 )
        output  =   fopen( "H:\\test.ac3", "wb+" );
    else if( code_id == AV_CODEC_ID_FLAC )
        output  =   fopen( "H:\\test.flac", "wb+" );
    else
        assert(0);

    FILE    *fp     =   fopen( "H:\\test.pcm", "rb" );
    int     i,  count   =   0;
    int16_t     intens[2];

    //
    while( 0 == feof(fp) )
    {
        ret = av_frame_make_writable(frame);
        if( ret < 0 )
            MYLOG( LOG::ERROR, "frame not writeable." );

        for( i = 0; i < frame->nb_samples; i++ )
        {
            fread( intens, 2, sizeof(int16_t), fp );

            // 多聲道這邊需要另外處理
            if( code_id == AV_CODEC_ID_AAC || code_id == AV_CODEC_ID_AC3 )
            {
                *((float*)(frame->data[0]) + i)   =   1.0 * intens[0] / INT16_MAX;
                *((float*)(frame->data[1]) + i)   =   1.0 * intens[1] / INT16_MAX;
            }
            else if( code_id == AV_CODEC_ID_MP3 )
            {
                *((int16_t*)(frame->data[0]) + i)   =   intens[0];
                *((int16_t*)(frame->data[1]) + i)   =   intens[1];
            }
            else if( code_id == AV_CODEC_ID_MP2 || code_id == AV_CODEC_ID_FLAC )
            {
                *((int16_t*)(frame->data[0]) + 2*i     )   =   intens[0];
                *((int16_t*)(frame->data[0]) + 2*i + 1 )   =   intens[1];
            }
        }
        frame->pts = count * (frame->nb_samples * 1000 / ctx->sample_rate);	// 沒加上這個也能動
        count++;
        encode( frame, code_id );
    }


    // flush
    encode( NULL, code_id );

    fclose(output);
    fclose(fp);
#endif
}






/*******************************************************************************
AudioEncode::get_pts()
********************************************************************************/
int64_t     AudioEncode::get_pts()
{
    if( frame == nullptr )
        MYLOG( LOG::ERROR, "frame is null." );

    return  frame->pts;
}






/*******************************************************************************
AudioEncode::get_frame()
********************************************************************************/
AVFrame*    AudioEncode::get_frame()
{
    AVCodecID code_id   =   ctx->codec_id; 

    int     ret;
    int     sp_count    =   0;

    static FILE *fp = fopen( "J:\\test.pcm", "rb" );
    int     i;
    int16_t     intens[2];

    if( feof(fp) != 0 )
    //if( frame_count > 400 )
        return nullptr;

    ret = av_frame_make_writable(frame);
    if( ret < 0 )
        MYLOG( LOG::ERROR, "frame not writeable." );
    
    for( i = 0; i < frame->nb_samples; i++ )
    {
        ret = fread( intens, 2, sizeof(int16_t), fp );
        if( ret == 0 )
        {
            //intens[0] = 0;
            //intens[1] = 0;
            break;
        }

        // 多聲道這邊需要另外處理
        if( code_id == AV_CODEC_ID_AAC || code_id == AV_CODEC_ID_AC3 )
        {
            *((float*)(frame->data[0]) + i)   =   1.0 * intens[0] / INT16_MAX;
            *((float*)(frame->data[1]) + i)   =   1.0 * intens[1] / INT16_MAX;
        }
        else if( code_id == AV_CODEC_ID_MP3 )
        {
            *((int16_t*)(frame->data[0]) + i)   =   intens[0];
            *((int16_t*)(frame->data[1]) + i)   =   intens[1];
        }
        else if( code_id == AV_CODEC_ID_MP2 || code_id == AV_CODEC_ID_FLAC )
        {
            *((int16_t*)(frame->data[0]) + 2*i     )   =   intens[0];
            *((int16_t*)(frame->data[0]) + 2*i + 1 )   =   intens[1];
        }
    }

    sp_count    =   i;

    if( i < frame->nb_samples )
    {
        // 有機會再改成memcpy或是其他版本
        for( ; i < frame->nb_samples; i++ )
        {
            // 多聲道這邊需要另外處理
            if( code_id == AV_CODEC_ID_AAC || code_id == AV_CODEC_ID_AC3 )
            {
                *((float*)(frame->data[0]) + i)   =   0;
                *((float*)(frame->data[1]) + i)   =   0;
            }
            else if( code_id == AV_CODEC_ID_MP3 )
            {
                *((int16_t*)(frame->data[0]) + i)   =   0;
                *((int16_t*)(frame->data[1]) + i)   =   0;
            }
            else if( code_id == AV_CODEC_ID_MP2 || code_id == AV_CODEC_ID_FLAC )
            {
                *((int16_t*)(frame->data[0]) + 2*i     )   =   0;
                *((int16_t*)(frame->data[0]) + 2*i + 1 )   =   0;
            }
        }
    }

    /*if( frame_count == 0 )
        frame->pts = 0;
    else 
        frame->pts += frame->nb_samples;  */
    // 考慮效能, 沒用 frame->pts = frame->nb_samples * frame_count; 的寫法
    frame->pts += sp_count; //frame->nb_samples;  

    frame_count++;

    return frame;
}




/*******************************************************************************
AudioEncode::send_frame()
********************************************************************************/
int     AudioEncode::send_frame() 
{
    static int samples_count = 0;

    if( frame != nullptr )
    {
        // 原本的 pts 拿來判斷應該丟 video 還是 audio frame.
        // 這邊會更改pts.
        //AVRational avr { 1, ctx->sample_rate };
        //frame->pts = av_rescale_q( samples_count, avr, ctx->time_base);
        //samples_count += frame->nb_samples; //  dst_nb_samples;
        // 看起來上面的程式碼沒影響 確認後刪除
    }

    int ret = Encode::send_frame();
    return ret;
}
