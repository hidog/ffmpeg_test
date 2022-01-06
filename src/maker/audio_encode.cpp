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

    // S16 packed, pcm[1] ���ϥ�.
    delete [] pcm[0]; 
    pcm[0]  =   nullptr;
}






/*******************************************************************************
AudioEncode::init()
********************************************************************************/
void    AudioEncode::init( int st_idx, AudioEncodeSetting setting, bool need_global_header )
{
    AVCodecID   code_id     =   setting.code_id;
    int         ret         =   0;

    Encode::init( st_idx, code_id );    

    // some codec need set bit rate.
    // ���Ҥ@�U�o��Ʊ�. ���� codec �|�۰ʲ��͹w�] bit rate.
    if( setting.code_id != AV_CODEC_ID_FLAC )
        ctx->bit_rate   =   setting.bit_rate;

    // format�i���,���䴩�׸�codec����.
    if( code_id == AV_CODEC_ID_MP3 )
        ctx->sample_fmt     =   AV_SAMPLE_FMT_S16P;
    else if( code_id == AV_CODEC_ID_AAC || code_id == AV_CODEC_ID_AC3 || code_id == AV_CODEC_ID_VORBIS )
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
    frame->pts              =   -frame->nb_samples;  // �@�Ө��������k, �ѦҨ��o frame ��code, �T�O�Ĥ@�� frame �� pts �O 0
    
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

    // �i�H�γo�Ө�ƨӪ�l��
    // swr_alloc_set_opts

    swr_ctx     =   swr_alloc();
    if( swr_ctx == nullptr ) 
        MYLOG( LOG::ERROR, "swr ctx is null." );

    // ��J�w�]��, ���ӦA�令�ʺA�M�w�Ѽ�
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

    if( pcm[0] != nullptr )
        MYLOG( LOG::ERROR, "pc is not null" );

    pcm_size    =   av_samples_get_buffer_size( NULL, ctx->channels, ctx->frame_size, AV_SAMPLE_FMT_S16, 0 );
    pcm[0]      =   new int16_t[pcm_size];

    if( pcm[0] == nullptr )
        MYLOG( LOG::ERROR, "pcm init fail." );
}






/*******************************************************************************
AudioEncode::encode()

�ثe�����
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

aac �榡, �C�� frame �n�[�Whead, �~��W�߼���
********************************************************************************/
char* AudioEncode::adts_head( int packetlen )
{
    packetlen   +=  7;

    static char packet[7];
    
    int profile = 2;
    int freqidx = 3; // 48000 hz. ��sample rate�n��۰ʳo�ӰѼ�
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

�ثe�����.
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

            // �h�n�D�o��ݭn�t�~�B�z
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
        frame->pts = count * (frame->nb_samples * 1000 / ctx->sample_rate);	// �S�[�W�o�Ӥ]���
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
AudioEncode::get_frame_from_file_test()

���եε{��, �|�q�ɮ�Ū�� pcm data.
********************************************************************************/
AVFrame*    AudioEncode::get_frame_from_file_test()
{
    AVCodecID code_id   =   ctx->codec_id; 

    int     ret;
    int     sp_count    =   0;

    static FILE *fp = fopen( "J:\\test.pcm", "rb" );

    //
    if( feof(fp) != 0 )
        return nullptr;

    ret = av_frame_make_writable(frame);
    if( ret < 0 )
        MYLOG( LOG::ERROR, "frame not writeable." );

    int     i;
    int16_t     intens[2];

    for( i = 0; i < frame->nb_samples; i++ )
    {
        ret = fread( intens, 2, sizeof(int16_t), fp );
        if( ret == 0 )
            break;
        
        // �h�n�D�o��ݭn�t�~�B�z
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

    // note: �b fread ��A�� feof �� i ���P�_
    // ���M�����| feof �P�_�٦����, �� fread Ū�쪺�O�ɮ׵���.
    if( i == 0 && feof(fp) != 0 )
        return nullptr;

    sp_count    =   i;

    if( i < frame->nb_samples )
    {
        // �����|�A�令memcpy�άO��L����
        for( ; i < frame->nb_samples; i++ )
        {
            // �h�n�D�o��ݭn�t�~�B�z
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
    // �Ҽ{�į�, �S�� frame->pts = frame->nb_samples * frame_count; ���g�k
    frame->pts  +=  sp_count; //frame->nb_samples;  
    frame_count++;

    return frame;
}






/*******************************************************************************
AudioEncode::get_frame_from_pcm_file()
********************************************************************************/
AVFrame*    AudioEncode::get_frame_from_pcm_file()
{
    static int   bytes_per_sample    =   av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    AVCodecID code_id   =   ctx->codec_id; 

    int     ret;
    int     sp_count    =   0;

    static FILE *fp     =   fopen( load_pcm_path.c_str(), "rb" );    

    //
    if( feof(fp) != 0 )
        return  nullptr;

    ret     =   av_frame_make_writable(frame);
    if( ret < 0 )
        MYLOG( LOG::ERROR, "frame not writeable." );   

    ret         =   fread( pcm[0], 1, pcm_size, fp );  // �����W�i�H�Q���� fread( pcm[0], sizeof(int16_t), channels * frame->nb_samples, fp );
    sp_count    =   ret / ctx->channels / bytes_per_sample;  

    if( ret == 0 && feof(fp) != 0 )
        return nullptr; // end of file.

    if( ret < pcm_size )    
        memset( pcm[0] + ret, 0, pcm_size - ret );

    ret     =   swr_convert( swr_ctx, frame->data, frame->nb_samples, (const uint8_t **)pcm, frame->nb_samples );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "convert fail." );

    frame->pts  +=  sp_count;
    frame_count++;

    return  frame;
}








/*******************************************************************************
AudioEncode::get_frame()
********************************************************************************/
AVFrame*    AudioEncode::get_frame()
{
    return  get_frame_from_pcm_file();
    //return  get_frame_from_file_test();
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





