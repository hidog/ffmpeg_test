#include "audio_decode.h"
#include "tool.h"

extern "C" {

#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

#include <libswresample/swresample.h>

} // end extern "C"






/*******************************************************************************
AudioDecode::AudioDecode()
********************************************************************************/
AudioDecode::AudioDecode()
    :   Decode()
{
    type  =   AVMEDIA_TYPE_AUDIO;
    //a_codec_id  =   AV_CODEC_ID_NONE;
}






/*******************************************************************************
AudioDecode::output_decode_info()
********************************************************************************/
void    AudioDecode::output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx )
{
    MYLOG( LOG::INFO, "audio dec name = %s", dec->name );
    MYLOG( LOG::INFO, "audio dec long name = %s", dec->long_name );
    MYLOG( LOG::INFO, "audio dec codec id = %s", avcodec_get_name(dec->id) );
    MYLOG( LOG::INFO, "audio bitrate = %d", dec_ctx->bit_rate );
}





/*******************************************************************************
AudioDecode::~AudioDecode()
********************************************************************************/
AudioDecode::~AudioDecode()
{
    end();
}





/*******************************************************************************
AudioDecode::open_codec_context()
********************************************************************************/
int     AudioDecode::open_codec_context( AVFormatContext *fmt_ctx )
{
    Decode::open_all_codec( fmt_ctx, type );
    //dec_ctx->thread_count = 4;
    return  SUCCESS;
}





/*******************************************************************************
AudioDecode::init()

av_get_channel_layout_nb_channels   轉成聲道數
av_get_default_channel_layout       轉成 layout
********************************************************************************/
int     AudioDecode::init()
{
    sample_rate     =   dec_ctx->sample_rate;
    sample_fmt      =   dec_ctx->sample_fmt;
    channel_layout  =   av_get_default_channel_layout( dec_ctx->channels );

    // 試著想要改變 sample rate, 但沒成功.                                                  
    // S16 改 S32, 需要修改pcm的部分. 需要找時間研究
    swr_ctx     =   swr_alloc_set_opts( swr_ctx,
                                        av_get_default_channel_layout(2), AV_SAMPLE_FMT_S16, sample_rate,  // output
                                        channel_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate,         // input 
                                        NULL, NULL );
    swr_init(swr_ctx);

    MYLOG( LOG::INFO, "audio sample format = %s", av_get_sample_fmt_name(sample_fmt) );
    MYLOG( LOG::INFO, "audio channel = %d, sample rate = %d", av_get_channel_layout_nb_channels(channel_layout), sample_rate );

    //
#ifdef FFMPEG_TEST
    //output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_QT, this );
    output_frame_func   =   std::bind( &AudioDecode::output_pcm, this );
#endif

    Decode::init();
    return  SUCCESS;
}






#ifdef FFMPEG_TEST
/*******************************************************************************
AudioDecode::set_output_audio_pcm_path()
********************************************************************************/
void    AudioDecode::set_output_audio_pcm_path( std::string _path )
{
    output_pcm_path =   _path;
}
#endif






#ifdef FFMPEG_TEST
/*******************************************************************************
AudioDecode::output_pcm()
********************************************************************************/
int     AudioDecode::output_pcm()
{
    static FILE *fp     =   fopen( output_pcm_path.c_str(), "wb+" );

    static constexpr int    out_channel =   2; // 目前預設輸出成兩聲道. 有空再改

    uint8_t     *data[2]    =   { 0 };  // S16 改 S32, 不確定是不是這邊的 array 要改成 4
                                        //int         byte_count     =   frame->nb_samples * 2 * 2;  // S16 改 S32, 改成 *4, 理論上資料量會增加, 但不確定是否改的是這邊
                                        // frame->nb_samples * 2 * 2     表示     分配樣本資料量 * 兩通道 * 每通道2位元組大小
    int         byte_count  =   av_samples_get_buffer_size( NULL, out_channel, frame->nb_samples, AV_SAMPLE_FMT_S16, 0 );

    unsigned char   *pcm    =   new uint8_t[byte_count];     

    if( pcm == nullptr )
        MYLOG( LOG::WARN, "pcm is null" );

    data[0]     =   pcm;    // 輸出格式為 AV_SAMPLE_FMT_S16(packet型別), 所以轉換後的 LR 兩通道都存在data[0]中
                            // 研究一下 S32 是不是存兩個資料
    int ret     =   swr_convert( swr_ctx,
                                 data, frame->nb_samples,                              //輸出 
                                 (const uint8_t**)frame->data, frame->nb_samples );    //輸入

    fwrite( pcm, 1, byte_count, fp );
    //MYLOG( LOG::DEBUG, "audio write %d. frame_count = %d", byte_count, frame_count );

    return  0;
}
#endif





/*******************************************************************************
AudioDecode::end()
********************************************************************************/
int     AudioDecode::end()
{
    if( swr_ctx != nullptr )
    {
        swr_close(swr_ctx);
        swr_ctx     =   nullptr;
    }
    
    sample_rate     =   0;
    channel_layout  =   0;
    sample_fmt      =   AV_SAMPLE_FMT_NONE;

    Decode::end();
    return  SUCCESS;
}





/*******************************************************************************
AudioDecode::output_audio_frame_info()
********************************************************************************/
void    AudioDecode::output_audio_frame_info()
{
    char    buf[AV_TS_MAX_STRING_SIZE]{0};
    int     per_sample  =   av_get_bytes_per_sample( static_cast<AVSampleFormat>(frame->format) );
    auto    pts_str     =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    MYLOG( LOG::INFO, "audio_frame n = %d, nb_samples = %d, pts : %s", frame_count, frame->nb_samples, pts_str );
}






/*******************************************************************************
AudioDecode::get_audio_nb_sample()
********************************************************************************/
int     AudioDecode::get_audio_nb_sample()
{
    return  dec_ctx->frame_size;
}





/*******************************************************************************
AudioDecode::get_audio_channel()
********************************************************************************/
int     AudioDecode::get_audio_channel()
{
    return  dec_ctx->channels;
    //return  stream->codecpar->channels;
}





/*******************************************************************************
AudioDecode::get_audio_channel_layout()
********************************************************************************/
int     AudioDecode::get_audio_channel_layout()
{
    return  dec_ctx->channel_layout;
}



/*******************************************************************************
AudioDecode::get_audio_sample_rate()
********************************************************************************/
int     AudioDecode::get_audio_sample_rate()
{
    //return  stream->codecpar->sample_rate;
    return  dec_ctx->sample_rate;
}



/*******************************************************************************
AudioDecode::get_audio_sample_format()
********************************************************************************/
int     AudioDecode::get_audio_sample_format()
{
    //return  stream->codecpar->format;
    return  static_cast<int>(dec_ctx->sample_fmt);
}





/*******************************************************************************
AudioDecode::output_audio_data()

有空來修改這邊 要能動態根據 mp4 檔案做調整

int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
要改成不同 sample rate 可以參考這個方式做調整
********************************************************************************/
AudioData   AudioDecode::output_audio_data()
{
    static constexpr int    out_channel =   2; // 目前預設輸出成兩聲道. 有空再改

    AudioData   ad;
    ad.pcm          =   nullptr;
    ad.bytes        =   0;
    ad.timestamp    =   0;


    int         byte_count  =   av_samples_get_buffer_size( NULL, out_channel, frame->nb_samples, AV_SAMPLE_FMT_S16, 0 );

    ad.pcm    =   new uint8_t[byte_count];
    if( ad.pcm == nullptr )
        MYLOG( LOG::WARN, "pcm is null" );

    uint8_t *data[2]    =   { 0 };
    data[0] =   ad.pcm;    // 輸出格式為 AV_SAMPLE_FMT_S16(packet型別), 所以轉換後的 LR 兩通道都存在data[0]中

    int ret     =   swr_convert( swr_ctx,
                                 data, frame->nb_samples,                              //輸出 
                                 (const uint8_t**)frame->data, frame->nb_samples );    //輸入

    //
    ad.bytes        =   byte_count;
    ad.timestamp    =   get_timestamp();

    return ad;
}





/*******************************************************************************
AudioDecode::get_timestamp()
********************************************************************************/
int64_t     AudioDecode::get_timestamp()
{
    AVRational  tb;
    tb.num  =   stream->time_base.num;
    tb.den  =   stream->time_base.den;

    double      dpts    =   frame->pts * av_q2d(tb);
    int64_t     ts      =   1000 * dpts; // ms

    return  ts;
}







/*******************************************************************************
AudioDecode::audio_info()
********************************************************************************/
int     AudioDecode::audio_info()
{
#if 0
    as_idx      =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0 );

    //
    AVStream    *audio_stream   =   fmt_ctx->streams[as_idx];
    if( audio_stream == nullptr )
    {
        MYLOG( LOG::INFO, "this stream has no audio stream" );
        return  SUCCESS;
    }

    //
    a_codec_id   =   fmt_ctx->streams[as_idx]->codecpar->codec_id;
    MYLOG( LOG::INFO, "code name = %s", avcodec_get_name(a_codec_id) );

    //
    channel     =   fmt_ctx->streams[as_idx]->codecpar->channels;
    sample_rate =   fmt_ctx->streams[as_idx]->codecpar->sample_rate;
    MYLOG( LOG::INFO, "channel = %d, sample rate = %d", channel, sample_rate );

    //
    double a_dur_ms = av_q2d( fmt_ctx->streams[as_idx]->time_base) * fmt_ctx->streams[as_idx]->duration;
    MYLOG( LOG::INFO, "frame duration = %lf ms", a_dur_ms );

#endif
    return  SUCCESS;
}
