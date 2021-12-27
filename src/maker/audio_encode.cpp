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
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

} // end extern "C"




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
    frame   =   nullptr;

    av_packet_free( &pkt );
    pkt     =   nullptr;

    avcodec_free_context( &ctx );
    ctx     =   nullptr;

    swr_free( &swr_ctx );
    swr_ctx     =   nullptr;

    //av_freep( &audio_data[0] );
    //av_freep( audio_data );
    //audio_data      =   nullptr;
    //audio_linesize  =   0;
}






/*******************************************************************************
AudioEncode::init()
********************************************************************************/
void    AudioEncode::init( int st_idx, AudioEncodeSetting setting, bool need_global_header )
{
    AVCodecID   code_id     =   setting.code_id;
    int         ret         =   0;

    // note 可以用 alloc_audio_frame 初始化 frame.

    Encode::init( st_idx, code_id );
    Encode::open();

    // some codec need set bit rate.
    // 驗證一下這件事情. 部分 codec 會自動產生預設 bit rate.
    if( setting.code_id != AV_CODEC_ID_FLAC )
        ctx->bit_rate   =   setting.bit_rate;

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
    ctx->sample_rate    =   setting.sample_rate; 
    ctx->channel_layout =   setting.channel_layout;
    ctx->channels       =   av_get_channel_layout_nb_channels(ctx->channel_layout);

    if( need_global_header == true )
        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // open ctx.1
    //AVDictionary *opt_arg = nullptr;
    //AVDictionary *opt = NULL;
    //av_dict_copy(&opt, opt_arg, 0);
    //ret     =   avcodec_open2( ctx, codec, &opt );

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
    frame->pts              =   -frame->nb_samples;  // 一個取巧的做法, 參考取得 frame 的code, 確保第一個 frame 的 pts 是 0
    
    // allocate the data buffers
    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "alloc buffer fail" );

    //
    init_swr(setting);
}






/*******************************************************************************
AudioEncode::init_swr()
********************************************************************************/
void    AudioEncode::init_swr( AudioEncodeSetting setting )
{
    int     ret     =   0;

    if( swr_ctx != nullptr )
        MYLOG( LOG::WARN, "swr_ctx is not null." );


    /* use for variable frame size.
    if ( ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = ctx->frame_size;*/

    // 可以用這個函數來初始化
    // swr_alloc_set_opts

    swr_ctx     =   swr_alloc();
    if( swr_ctx == nullptr ) 
        MYLOG( LOG::ERROR, "swr ctx is null." );

    // 輸入預設值, 未來再改成動態決定參數
    av_opt_set_int        ( swr_ctx, "in_channel_count",   2,                   0 );
    av_opt_set_int        ( swr_ctx, "in_sample_rate",     48000,               0 );
    av_opt_set_sample_fmt ( swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16,   0 );
    
    av_opt_set_int        ( swr_ctx, "out_channel_count",  ctx->channels,       0 );
    av_opt_set_int        ( swr_ctx, "out_sample_rate",    ctx->sample_rate,    0 );
    av_opt_set_sample_fmt ( swr_ctx, "out_sample_fmt",     ctx->sample_fmt,     0 );

    //
    ret     =   swr_init( swr_ctx );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "swr init fail." );

    //
    //if( src_frame != nullptr )
        //MYLOG( LOG::WARN, "src_frame not null." );

    //src_frame   =   alloc_audio_frame( AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_STEREO, 48000, nb_samples);
    if( src_data != nullptr )
        MYLOG( LOG::ERROR, "src_data not null." );
    ret     =   av_samples_alloc_array_and_samples( &src_data, &src_linesize, 2, 48000, AV_SAMPLE_FMT_S16, 0 );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "alloc src data fail. " );

    if( audio_data != nullptr )
        MYLOG( LOG::WARN, "audio_data not null." );
    ret     =   av_samples_alloc_array_and_samples( &audio_data, &audio_linesize, ctx->channels, ctx->sample_rate,  ctx->sample_fmt, 0 );
    if( ret < 0 )
        MYLOG( LOG::ERROR, "alloc src data fail. " );
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

    //if( frame_count > 600 )
      //  return  nullptr;

    //
    if( feof(fp) != 0 )
        return nullptr;

    ret = av_frame_make_writable(frame);
    if( ret < 0 )
        MYLOG( LOG::ERROR, "frame not writeable." );
    
#if 1
    /* convert samples from native format to destination codec format, using the resampler */
    /* compute destination number of samples */
    int dst_nb_samples = av_rescale_rnd( swr_get_delay(swr_ctx, ctx->sample_rate) + frame->nb_samples, ctx->sample_rate, ctx->sample_rate, AV_ROUND_UP);
    //av_assert0(dst_nb_samples == frame->nb_samples);

    /* when we pass a frame to the encoder, it may keep a reference to it
    * internally;
    * make sure we do not overwrite it here
    */
    ret = av_frame_make_writable(frame);
    if (ret < 0)
      exit(1);

    int         byte_count  =   2*2*1152; //av_samples_get_buffer_size( NULL, 2, 1152, AV_SAMPLE_FMT_S16, 0 );
    //int byte_count = 1024 * 2 * sizeof(int16_t);  // 2 is channels
    //int byte_count = audio_linesize;

    uint8_t* ptr[2];
    ptr[0] =  new uint8_t[ byte_count ];
    //ptr[1] = new uint8_t[ byte_count ];
    ret = fread( ptr[0], 1, byte_count, fp );
    //ret = fread( ptr[1], 1, byte_count, fp );

    //ret = fread( src_data[0], 1, byte_count, fp );
    sp_count = 1152; // ret/2;


    //ret = av_samples_alloc(audio_data, &audio_linesize, 2, dst_nb_samples, ctx->sample_fmt, 1);

    /* convert to destination format */
    /*ret = swr_convert( swr_ctx,
        audio_data, audio_linesize,
      (const uint8_t **)src_data, byte_count );*/

      /*ret = swr_convert( swr_ctx,
      frame->data, 1152,
          (const uint8_t **)ptr, 1152 );*/

          ret = swr_convert( swr_ctx,
          audio_data, 1152,
          (const uint8_t **)ptr, 1152 );

          memcpy( frame->data[0], audio_data[0], 1152*2 );
          memcpy( frame->data[1], audio_data[1], 1152*2 );


      delete ptr[0];
      //delete ptr[1];

    if (ret < 0) {
    fprintf(stderr, "Error while converting\n");
    exit(1);
    }

    //frame = ost->frame;


    //ret     =   fread( 


      //  swr_convert
#else

    int     i;
    int16_t     intens[2];

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

    // note: 在 fread 後再用 feof 跟 i 做判斷
    // 不然有機會 feof 判斷還有資料, 但 fread 讀到的是檔案結尾.
    if( i == 0 && feof(fp) != 0 )
        return nullptr;

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
#endif


    /*if( frame_count == 0 )
        frame->pts = 0;
    else 
        frame->pts += frame->nb_samples;  */
    // 考慮效能, 沒用 frame->pts = frame->nb_samples * frame_count; 的寫法
    frame->pts  +=  sp_count; //frame->nb_samples;  

    frame_count++;

    return frame;
}




/*******************************************************************************
AudioEncode::send_frame()
********************************************************************************/
int     AudioEncode::send_frame() 
{
    static int samples_count    =   0;
    int     ret     =   Encode::send_frame();
    return  ret;
}






#if 0

/**
* @example resampling_audio.c
* libswresample API use example.
*/

#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>

static int get_format_from_sample_fmt(const char **fmt,
    enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = NULL;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
        "Sample format %s not supported as output format\n",
        av_get_sample_fmt_name(sample_fmt));
    return AVERROR(EINVAL);
}

/**
* Fill dst buffer with nb_samples, generated starting from t.
*/
static void fill_samples(double *dst, int nb_samples, int nb_channels, int sample_rate, double *t)
{
    int i, j;
    double tincr = 1.0 / sample_rate, *dstp = dst;
    const double c = 2 * M_PI * 440.0;

    /* generate sin tone with 440Hz frequency and duplicated channels */
    for (i = 0; i < nb_samples; i++) {
        *dstp = sin(c * *t);
        for (j = 1; j < nb_channels; j++)
            dstp[j] = dstp[0];
        dstp += nb_channels;
        *t += tincr;
    }
}

int main(int argc, char **argv)
{
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO, dst_ch_layout = AV_CH_LAYOUT_SURROUND;
    int src_rate = 48000, dst_rate = 44100;


    uint8_t **src_data = NULL, **dst_data = NULL;
    int src_linesize, dst_linesize;


    int src_nb_channels = 0, dst_nb_channels = 0;




    int src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_DBL, dst_sample_fmt = AV_SAMPLE_FMT_S16;
    const char *dst_filename = NULL;
    FILE *dst_file;
    int dst_bufsize;
    const char *fmt;
    struct SwrContext *swr_ctx;
    double t;
    int ret;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s output_file\n"
            "API example program to show how to resample an audio stream with libswresample.\n"
            "This program generates a series of audio frames, resamples them to a specified "
            "output format and rate and saves them to an output file named output_file.\n",
            argv[0]);
        exit(1);
    }
    dst_filename = argv[1];

    dst_file = fopen(dst_filename, "wb");
    if (!dst_file) {
        fprintf(stderr, "Could not open destination file %s\n", dst_filename);
        exit(1);
    }

    /* create resampler context */
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* set options */
    av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate",       src_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate",       dst_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

    /* initialize the resampling context */
    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        goto end;
    }

    /* allocate source and destination samples buffers */

    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels,
        src_nb_samples, src_sample_fmt, 0);


    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        goto end;
    }

    /* compute the number of converted samples: buffering is avoided
    * ensuring that the output buffer will contain at least all the
    * converted input samples */
    max_dst_nb_samples = dst_nb_samples =
        av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

    /* buffer is going to be directly written to a rawaudio file, no alignment */
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
        dst_nb_samples, dst_sample_fmt, 0);


    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");
        goto end;
    }




    t = 0;
    do {
        /* generate synthetic audio */
        fill_samples( (double *)src_data[0], src_nb_samples, src_nb_channels, src_rate, &t );



        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, src_rate) +
            src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            av_freep(&dst_data[0]);
            ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
                dst_nb_samples, dst_sample_fmt, 1);
            if (ret < 0)
                break;
            max_dst_nb_samples = dst_nb_samples;
        }



        /* convert to destination format */
        ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)src_data, src_nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            goto end;
        }


        dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
            ret, dst_sample_fmt, 1);


        if (dst_bufsize < 0) {
            fprintf(stderr, "Could not get sample buffer size\n");
            goto end;
        }
        printf("t:%f in:%d out:%d\n", t, src_nb_samples, ret);
        fwrite(dst_data[0], 1, dst_bufsize, dst_file);
    } while (t < 10);

    if ((ret = get_format_from_sample_fmt(&fmt, dst_sample_fmt)) < 0)
        goto end;
    fprintf(stderr, "Resampling succeeded. Play the output file with the command:\n"
        "ffplay -f %s -channel_layout %"PRId64" -channels %d -ar %d %s\n",
        fmt, dst_ch_layout, dst_nb_channels, dst_rate, dst_filename);

end:
    fclose(dst_file);

    if (src_data)
        av_freep(&src_data[0]);
    av_freep(&src_data);

    if (dst_data)
        av_freep(&dst_data[0]);
    av_freep(&dst_data);

    swr_free(&swr_ctx);
    return ret < 0;
}
#endif