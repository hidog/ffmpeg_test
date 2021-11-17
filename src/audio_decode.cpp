#include "audio_decode.h"
#include "tool.h"

extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libswresample/swresample.h>

} // end extern "C"



/*******************************************************************************
AudioDecode::AudioDecode()
********************************************************************************/
AudioDecode::AudioDecode()
    :   Decode()
{}






/*******************************************************************************
AudioDecode::~AudioDecode()
********************************************************************************/
AudioDecode::~AudioDecode()
{}





/*******************************************************************************
AudioDecode::open_codec_context()
********************************************************************************/
int     AudioDecode::open_codec_context( int stream_index, AVFormatContext *fmt_ctx )
{
    Decode::open_codec_context( stream_index, fmt_ctx, type );
    return  SUCCESS;
}





/*******************************************************************************
AudioDecode::init()
********************************************************************************/
int     AudioDecode::init()
{
    // �յ۷Q�n���� sample rate, ���S���\.                                                  
    swr_ctx     =   swr_alloc_set_opts( swr_ctx, 
                                        av_get_default_channel_layout(2), AV_SAMPLE_FMT_S16, 48000,                  // output
                                        dec_ctx->channel_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate,          // input
                                        NULL, NULL );
    swr_init(swr_ctx);

    /*destMs = av_q2d(pFmtCtx->streams[audioindex]->time_base)*1000*pFmtCtx->streams[audioindex]->duration;
    qDebug()<<"�줸�t�v:"<<acodecCtx->bit_rate;
    qDebug()<<"�榡:"<<acodecCtx->sample_fmt;
    qDebug()<<"�q�D:"<<acodecCtx->channels;
    qDebug()<<"���˲v:"<<acodecCtx->sample_rate;
    qDebug()<<"�ɪ�:"<<destMs;
    qDebug()<<"�ѽX��:"<<acodec->name;*/

    Decode::init();
    return  SUCCESS;
}





/*******************************************************************************
AudioDecode::end()
********************************************************************************/
int     AudioDecode::end()
{
    Decode::end();
    return  SUCCESS;
}






#if 0
��z�ݭn����T
/*******************************************************************************
AudioDecode::output_frame()
********************************************************************************/
int     AudioDecode::output_frame()
{
    char    buf[AV_TS_MAX_STRING_SIZE]{0};
    int     per_sample  =   av_get_bytes_per_sample( static_cast<AVSampleFormat>(frame->format) );
    size_t  unpadded_linesize   =   frame->nb_samples * per_sample;

    auto str    =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    printf( "audio_frame n : %d nb_samples : %d pts : %s\n", frame_count++, frame->nb_samples, str );
    //av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));

    /* 
    Write the raw audio data samples of the first plane. This works
    fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
    most audio decoders output planar audio, which uses a separate
    plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
    In other words, this code will write only the first audio channel
    in these cases.
    You should use libswresample or libavfilter to convert the frame
    to packed data. 
    */
    fwrite( frame->extended_data[0], 1, unpadded_linesize, dst_fp );
    return 0;
}
#endif







#if 0
��z�ݭn����T
/*******************************************************************************
AudioDecode::print_finish_message()
********************************************************************************/
void    AudioDecode::print_finish_message()
{
    int     ret     =   0;

    AVSampleFormat  sfmt        =   dec_ctx->sample_fmt;
    int             n_channels  =   dec_ctx->channels;
    const char      *fmt        =   nullptr;
    
    auto flag   =   av_sample_fmt_is_planar(sfmt);
    if( flag > 0 )
    {
        const char  *packed =   av_get_sample_fmt_name(sfmt);
        printf( "Warning: the sample format the decoder produced is planar "
                "(%s). This example will output the first channel only.\n", packed ? packed : "?");
        sfmt        =   av_get_packed_sample_fmt(sfmt);
        n_channels  =   1;
    }
    
    //
    ret =   get_format_from_sample_fmt( &fmt, sfmt );
    
    if( ret >= 0 )
    {
        printf( "Play the output audio file with the command:\n"
                "ffplay -f %s -ac %d -ar %d %s\n", fmt, n_channels, dec_ctx->sample_rate, dst_file.c_str() );
    }
    
}
#endif







#if 0
��z�ݭn����T
/*******************************************************************************
AudioDecode::get_format_from_sample_fmt()
********************************************************************************/
int     AudioDecode::get_format_from_sample_fmt( const char **fmt, enum AVSampleFormat sample_fmt )
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

    for( i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++ ) 
    {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) 
        {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return SUCCESS;
        }
    }

    auto str =  av_get_sample_fmt_name(sample_fmt);
    ERRLOG( "sample format %s is not supported as output format", str );
    return  ERROR;
}
#endif





/*******************************************************************************
AudioDecode::output_audio_data()
********************************************************************************/
AudioData   AudioDecode::output_audio_data()
{
    AudioData   ad { nullptr, 0 };

    uint8_t *data[2] = { 0 };
    int byteCnt = frame->nb_samples * 2 * 2;

    unsigned char *pcm = new uint8_t[byteCnt];     //frame->nb_samples*2*2��ܤ��t�˥���ƶq*��q�D*�C�q�D2�줸�դj�p

    data[0] = pcm;  //��X�榡��AV_SAMPLE_FMT_S16(packet���O),�ҥH�ഫ�᪺LR��q�D���s�bdata[0]��

    int ret = swr_convert( swr_ctx,
                           data, frame->nb_samples,                              //��X
                           (const uint8_t**)frame->data, frame->nb_samples );    //��J

    ad.pcm = pcm;
    ad.bytes = byteCnt;

    return ad;
}
