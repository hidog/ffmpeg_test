#include "sub_decode.h"
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
SubDecode::SubDecode()
********************************************************************************/
SubDecode::SubDecode()
    :   Decode()
{
    AVMediaType   type  =   AVMEDIA_TYPE_SUBTITLE; 
}






/*******************************************************************************
SubDecode::output_decode_info()
********************************************************************************/
void    SubDecode::output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx )
{
    MYLOG( LOG::INFO, "sub dec name = %s", dec->name );
    MYLOG( LOG::INFO, "sub dec long name = %s", dec->long_name );
    MYLOG( LOG::INFO, "sub dec codec id = %s", avcodec_get_name(dec->id) );
    //MYLOG( LOG::INFO, "audio bitrate = %d", dec_ctx->bit_rate );
}




/*******************************************************************************
SubDecode::~SubDecode()
********************************************************************************/
SubDecode::~SubDecode()
{}





/*******************************************************************************
SubDecode::open_codec_context()
********************************************************************************/
int     SubDecode::open_codec_context( int stream_index, AVFormatContext *fmt_ctx )
{
    if( stream_index < 0 )
    {
        MYLOG( LOG::INFO, "no sub stream" );
        return  SUCCESS;
    }

    Decode::open_codec_context( stream_index, fmt_ctx, type );
    //dec_ctx->thread_count = 4;
    return  SUCCESS;
}





/*******************************************************************************
SubDecode::init()
********************************************************************************/
int     SubDecode::init()
{
#if 0
    sample_rate     =   dec_ctx->sample_rate;
    sample_fmt      =   dec_ctx->sample_fmt;
    channel_layout  =   dec_ctx->channel_layout;

    assert( dec_ctx->sample_fmt == AV_SAMPLE_FMT_FLTP ); // �p�G�J�줣�P��  �b�ݬO���O�n�վ�audio output��sample size

    // �յ۷Q�n���� sample rate, ���S���\.                                                  
    // S16 �� S32, �ݭn�ק�pcm������. �ݭn��ɶ���s
    swr_ctx     =   swr_alloc_set_opts( swr_ctx, 
                                        av_get_default_channel_layout(2), AV_SAMPLE_FMT_S16, sample_rate,           // output
                                        dec_ctx->channel_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate,         // input
                                        NULL, NULL );
    swr_init(swr_ctx);

    MYLOG( LOG::INFO, "audio sample format = %s", av_get_sample_fmt_name(sample_fmt) );
    MYLOG( LOG::INFO, "audio channel = %d, sample rate = %d", channel_layout, sample_rate );

    Decode::init();
#endif
    return  SUCCESS;
}





/*******************************************************************************
SubDecode::end()
********************************************************************************/
int     SubDecode::end()
{
    Decode::end();
    return  SUCCESS;
}



/*******************************************************************************
SubDecode::output_audio_frame_info()
********************************************************************************/
void    SubDecode::output_sub_frame_info()
{
    /*char    buf[AV_TS_MAX_STRING_SIZE]{0};
    int     per_sample  =   av_get_bytes_per_sample( static_cast<AVSampleFormat>(frame->format) );
    auto    pts_str     =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    MYLOG( LOG::INFO, "audio_frame n = %d, nb_samples = %d, pts : %s", frame_count++, frame->nb_samples, pts_str );*/
}





/*******************************************************************************
SubDecode::output_audio_data()
********************************************************************************/
SubData   SubDecode::output_sub_data()
{
#if 0
    AudioData   ad { nullptr, 0 };

    // ���Ũӭק�o�� �n��ʺA�ھ� mp4 �ɮװ��վ�

    uint8_t     *data[2]    =   { 0 };  // S16 �� S32, ���T�w�O���O�o�䪺 array �n�令 4
    int         byte_count     =   frame->nb_samples * 2 * 2;  // S16 �� S32, �令 *4, �z�פW��ƶq�|�W�[, �����T�w�O�_�諸�O�o��

    unsigned char   *pcm    =   new uint8_t[byte_count];     // frame->nb_samples * 2 * 2     ���     ���t�˥���ƶq * ��q�D * �C�q�D2�줸�դj�p

    if( pcm == nullptr )
        MYLOG( LOG::WARN, "pcm is null" );

    data[0]     =   pcm;    // ��X�榡��AV_SAMPLE_FMT_S16(packet���O),�ҥH�ഫ�᪺ LR ��q�D���s�bdata[0]��
    int ret     =   swr_convert( swr_ctx,
                                 data, sample_rate,                                    //��X
                                 //data, frame->nb_samples,                              //��X
                                 (const uint8_t**)frame->data, frame->nb_samples );    //��J

    ad.pcm      =   pcm;
    ad.bytes    =   byte_count;

    return ad;
#endif
    SubData     sd { 0 };
    return  sd;
}
