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

    assert( dec_ctx->sample_fmt == AV_SAMPLE_FMT_FLTP ); // 如果遇到不同的  在看是不是要調整audio output的sample size

    // 試著想要改變 sample rate, 但沒成功.                                                  
    // S16 改 S32, 需要修改pcm的部分. 需要找時間研究
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

    // 有空來修改這邊 要能動態根據 mp4 檔案做調整

    uint8_t     *data[2]    =   { 0 };  // S16 改 S32, 不確定是不是這邊的 array 要改成 4
    int         byte_count     =   frame->nb_samples * 2 * 2;  // S16 改 S32, 改成 *4, 理論上資料量會增加, 但不確定是否改的是這邊

    unsigned char   *pcm    =   new uint8_t[byte_count];     // frame->nb_samples * 2 * 2     表示     分配樣本資料量 * 兩通道 * 每通道2位元組大小

    if( pcm == nullptr )
        MYLOG( LOG::WARN, "pcm is null" );

    data[0]     =   pcm;    // 輸出格式為AV_SAMPLE_FMT_S16(packet型別),所以轉換後的 LR 兩通道都存在data[0]中
    int ret     =   swr_convert( swr_ctx,
                                 data, sample_rate,                                    //輸出
                                 //data, frame->nb_samples,                              //輸出
                                 (const uint8_t**)frame->data, frame->nb_samples );    //輸入

    ad.pcm      =   pcm;
    ad.bytes    =   byte_count;

    return ad;
#endif
    SubData     sd { 0 };
    return  sd;
}
