#include "audio_encode.h"

#include <stdio.h>
#include "tool.h"


extern "C" {

#include <libavcodec/avcodec.h>
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
{
    end();
}







/*******************************************************************************
AudioEncode::list_sample_format()
********************************************************************************/
void    AudioEncode::list_sample_format( AVCodecID code_id )
{
    const AVCodec           *codec   =   avcodec_find_encoder(code_id);
    const AVSampleFormat    *p_fmt   =   codec->sample_fmts;

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
    const AVCodec*  codec   =   avcodec_find_encoder(code_id);
    const int*      p_smp   =   codec->supported_samplerates;

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
    const AVCodec*     codec   =   avcodec_find_encoder(code_id);

    const AVChannelLayout  *p_cl    =   nullptr;
    uint64_t    best_ch_layout      =   0;
    int         best_nb_channels    =   0;

    p_cl    =   codec->ch_layouts;
    if( p_cl == nullptr )
    {
        printf("can not list.\n");
        return;
    }

    while( p_cl != 0 ) 
    {
        int nb_channels = p_cl->nb_channels;
        printf( "%s support channel layout %d\n", avcodec_get_name(code_id), nb_channels );
        p_cl++;
    }
}







/*******************************************************************************
AudioEncode::select_channel_layout()
********************************************************************************/
int     AudioEncode::select_channel_layout( AVCodec *codec )
{
    const AVChannelLayout  *p_cl    =   nullptr;
    //uint64_t    best_ch_layout      =   0;
    int         best_nb_channels    =   0;

    if( codec->channel_layouts == nullptr )
        return AV_CH_LAYOUT_STEREO;

    p_cl    =   codec->ch_layouts;
    while( p_cl != 0 ) 
    {
        int     nb_channels     =   p_cl->nb_channels;

        if( nb_channels > best_nb_channels ) 
        {
            //best_ch_layout      =   *p_cl;
            best_nb_channels    =   nb_channels;
        }
        p_cl++;
    }

    //return best_ch_layout;
    return best_nb_channels;
}








/*******************************************************************************
AudioEncode::end()
********************************************************************************/
void    AudioEncode::end()
{
    if( frame_count > 0 )
    {
        MYLOG( LOG::L_INFO, "audio encode %d frames.", frame_count );
        int64_t     duration_time   =   1000LL * ctx->frame_size * frame_count / ctx->sample_rate; // ms
        int64_t     ms              =   duration_time % 1000;
        int64_t     sec             =   duration_time / 1000 % 60;
        int64_t     minute          =   duration_time / 1000 / 60 % 60;
        int64_t     hour            =   duration_time / 1000 / 60 / 60;
        MYLOG( LOG::L_INFO, "audio encode duration = [ %2lld : %2lld : %2lld . %3lld", hour, minute, sec, ms );
    }

    Encode::end();

#ifdef FFMPEG_TEST
    if( swr_ctx != nullptr )
    {
        swr_free( &swr_ctx );
        swr_ctx     =   nullptr;
    }

    // S16 packed, pcm[1] 未使用.
    if( pcm[0] != nullptr )
    {
        delete [] pcm[0]; 
        pcm[0]  =   nullptr;
    }
#endif
}




/*******************************************************************************
AudioEncode::init()
********************************************************************************/
void    AudioEncode::init( int st_idx, AudioEncodeSetting setting, bool need_global_header )
{
#ifdef FFMPEG_TEST
    load_pcm_path   =   setting.load_pcm_path;
#endif

    Encode::init( st_idx, setting.code_id );

    //
    AVCodecID   code_id     =   setting.code_id;

    // some codec need set bit rate.
    if( code_id != AV_CODEC_ID_FLAC )
        ctx->bit_rate   =   setting.bit_rate;

    // format可更改,但支援度跟codec有關.
    if( code_id == AV_CODEC_ID_MP3 )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_S16P;
    else if( code_id == AV_CODEC_ID_AAC || code_id == AV_CODEC_ID_AC3 || code_id == AV_CODEC_ID_VORBIS )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_FLTP;
    else if( code_id == AV_CODEC_ID_MP2 || code_id == AV_CODEC_ID_FLAC )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_S16;
    else    
        MYLOG( LOG::L_ERROR, "un handle codec" );    

    if( false == check_sample_fmt( (AVCodec*)codec, ctx->sample_fmt ) )
        MYLOG( LOG::L_ERROR, "fmt fail." );

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

    int     ret     =   avcodec_open2( ctx, codec, nullptr );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "open fail." );

    //
    if( ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE )
        MYLOG( LOG::L_WARN, "need set to 10000" );

#ifdef FFMPEG_TEST
    // set param to frame.
    frame->nb_samples       =   ctx->frame_size;
    frame->format           =   ctx->sample_fmt;
    frame->channel_layout   =   ctx->channel_layout;
    frame->sample_rate      =   ctx->sample_rate;
    frame->pts              =   0; // -frame->nb_samples;  // 一個取巧的做法, 參考取得 frame 的code, 確保第一個 frame 的 pts 是 0
    
    // allocate the data buffers
    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "alloc buffer fail" );

    //
    init_swr(setting);
#endif
}





#ifdef FFMPEG_TEST
/*******************************************************************************
AudioEncode::init_swr()
********************************************************************************/
void    AudioEncode::init_swr( AudioEncodeSetting setting )
{
    int     ret     =   0;

    if( swr_ctx != nullptr )
        MYLOG( LOG::L_WARN, "swr_ctx is not null." );

    /* use for variable frame size.
    if ( ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = ctx->frame_size;*/

    // 可以用這個函數來初始化
    // swr_alloc_set_opts

    swr_ctx     =   swr_alloc();
    if( swr_ctx == nullptr ) 
        MYLOG( LOG::L_ERROR, "swr ctx is null." );

    // 輸入預設值, 未來再改成動態決定參數
    AVSampleFormat  sample_fmt  =   static_cast<AVSampleFormat>(setting.sample_fmt);
    int     channel     =   av_get_channel_layout_nb_channels( setting.channel_layout );

    av_opt_set_int        ( swr_ctx, "in_channel_count",   channel,                0 );
    av_opt_set_int        ( swr_ctx, "in_sample_rate",     setting.sample_rate,    0 );
    av_opt_set_sample_fmt ( swr_ctx, "in_sample_fmt",      sample_fmt,             0 );
    
    av_opt_set_int        ( swr_ctx, "out_channel_count",  ctx->channels,       0 );
    av_opt_set_int        ( swr_ctx, "out_sample_rate",    ctx->sample_rate,    0 );
    av_opt_set_sample_fmt ( swr_ctx, "out_sample_fmt",     ctx->sample_fmt,     0 );

    //
    ret     =   swr_init( swr_ctx );
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "swr init fail." );

    if( pcm[0] != nullptr )
        MYLOG( LOG::L_ERROR, "pc is not null" );

    pcm_size    =   av_samples_get_buffer_size( NULL, ctx->channels, ctx->frame_size, AV_SAMPLE_FMT_S16, 0 );
    pcm[0]      =   new int16_t[pcm_size];

    if( pcm[0] == nullptr )
        MYLOG( LOG::L_ERROR, "pcm init fail." );
}
#endif




#ifdef FFMPEG_TEST
/*******************************************************************************
AudioEncode::encode()

目前不能動
********************************************************************************/
void    AudioEncode::encode_test()
{
    int     ret;

    ret     =   avcodec_send_frame( ctx, frame );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "send fail." );

    while( ret >= 0 ) 
    {
        ret     =   avcodec_receive_packet( ctx, pkt );
        if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
            return;
        else if (ret < 0) 
            MYLOG( LOG::L_ERROR, "recv fail." );

        printf("write data %d...\n", pkt->size );

        //if( code_id == AV_CODEC_ID_AAC )
            //fwrite( adts_head(pkt->size), 1, 7, output );

        //fwrite( pkt->data, 1, pkt->size, output );
        av_packet_unref(pkt);
    }
}
#endif






#ifdef FFMPEG_TEST
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
#endif





#ifdef FFMPEG_TEST
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
            MYLOG( LOG::L_ERROR, "frame not writeable." );

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
#endif






/*******************************************************************************
AudioEncode::get_pts()
********************************************************************************/
int64_t     AudioEncode::get_pts()
{
    if( frame == nullptr )
        MYLOG( LOG::L_ERROR, "frame is null." );
    return  frame->pts;
}





#ifdef FFMPEG_TEST
/*******************************************************************************
AudioEncode::get_frame_from_file_test()

測試用程式, 會從檔案讀取 pcm data.
********************************************************************************/
void    AudioEncode::get_frame_from_file_test()
{
    AVCodecID code_id   =   ctx->codec_id; 

    int     ret;
    int     sp_count    =   0;

    static FILE *fp     =   fopen( "J:\\test.pcm", "rb" );

    //
    if( feof(fp) != 0 )
    {
        eof_flag    =   true;
        return;
    }

    ret     =   av_frame_make_writable(frame);
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "frame not writeable." );

    int     i;
    int16_t     intens[2];

    for( i = 0; i < frame->nb_samples; i++ )
    {
        ret     =   fread( intens, 2, sizeof(int16_t), fp );
        if( ret == 0 )
            break;
        
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
    {
        eof_flag    =   true;
        return;
    }

    sp_count    =   i;

    if( i < frame->nb_samples )
    {
        // 有機會再改成memcpy或是其他版本
        for( ; i < frame->nb_samples; i++ )
        {
            // 多聲道這邊需要另外處理
            if( code_id == AV_CODEC_ID_AAC || code_id == AV_CODEC_ID_AC3 || code_id == AV_CODEC_ID_VORBIS )
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
    frame->pts  +=  sp_count; //frame->nb_samples;  
    frame_count++;
}
#endif






#ifdef FFMPEG_TEST
/*******************************************************************************
AudioEncode::get_frame_from_pcm_file()
********************************************************************************/
void    AudioEncode::get_frame_from_pcm_file()
{
    static int  bytes_per_sample   =   av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    static int  sp_count  =   pcm_size / ctx->channels / bytes_per_sample;
    AVCodecID   code_id   =   ctx->codec_id; 
    int     ret;

    static FILE *fp     =   fopen( load_pcm_path.c_str(), "rb" );    

    //
    if( feof(fp) != 0 )
    //if( frame_count > 1300 )
    {
        eof_flag    =   true;
        return;
    }

    ret =   av_frame_make_writable(frame);
    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "frame not writeable." );   

    ret =   fread( pcm[0], 1, pcm_size, fp );  // 概念上可以想像成 fread( pcm[0], sizeof(int16_t), channels * frame->nb_samples, fp );

    if( ret == 0 && feof(fp) != 0 )
    {
        eof_flag    =   true;
        MYLOG( LOG::L_WARN, "should not run here." );
        return; // end of file.
    }

    if( ret < pcm_size )
        memset( pcm[0] + ret, 0, pcm_size - ret );

    ret     =   swr_convert( swr_ctx, frame->data, frame->nb_samples, (const uint8_t **)pcm, frame->nb_samples );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "convert fail." );

    //
    frame->pts  =  frame_count * sp_count;
    frame_count++;
}
#endif









/*******************************************************************************
AudioEncode::next_frame()
********************************************************************************/
void    AudioEncode::next_frame()
{
#ifdef FFMPEG_TEST
    get_frame_from_pcm_file();
    //get_frame_from_file_test();
#else
    frame   =   encode::get_audio_frame();
    if( frame == nullptr )
        eof_flag    =   true;
#endif
}





/*******************************************************************************
AudioEncode::unref_data()
********************************************************************************/
void    AudioEncode::unref_data()
{
#ifndef FFMPEG_TEST
    av_frame_free( &frame );
#endif
    Encode::unref_data();
}

