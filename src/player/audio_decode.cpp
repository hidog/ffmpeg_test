#include "audio_decode.h"
#include "tool.h"

extern "C" {

#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>

} // end extern "C"






/*******************************************************************************
AudioDecode::AudioDecode()
********************************************************************************/
AudioDecode::AudioDecode()
    :   Decode(AVMEDIA_TYPE_AUDIO)
{
    sample_fmt  =   AV_SAMPLE_FMT_NONE;
}






/*******************************************************************************
AudioDecode::output_decode_info()
********************************************************************************/
void    AudioDecode::output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx )
{
    MYLOG( LOG::L_INFO, "audio dec name = %s", dec->name );
    MYLOG( LOG::L_INFO, "audio dec long name = %s", dec->long_name );
    MYLOG( LOG::L_INFO, "audio dec codec id = %s", avcodec_get_name(dec->id) );
    MYLOG( LOG::L_INFO, "audio bitrate = %d", dec_ctx->bit_rate );
}





/*******************************************************************************
AudioDecode::~AudioDecode()
********************************************************************************/
AudioDecode::~AudioDecode()
{
    end();
}





/*******************************************************************************
AudioDecode::init()

av_get_channel_layout_nb_channels   轉成聲道數
av_get_default_channel_layout       轉成 layout
********************************************************************************/
int     AudioDecode::init()
{
    AVChannelLayout     out_layout, in_layout;
    int     res     =   0;

    sample_rate     =   dec_ctx->sample_rate;
    sample_fmt      =   dec_ctx->sample_fmt;
    channel_layout  =   av_get_default_channel_layout( dec_ctx->channels );

    // 要改變 sample rate 可以參考 ffmpeg 官方範例.  resampling_audio                                              
    // S16 改 S32, 需要修改pcm的部分. 需要找時間研究

    dst_fmt =   sample_fmt;
    switch( sample_fmt )
    {
    case AV_SAMPLE_FMT_U8P:
        dst_fmt     =   AV_SAMPLE_FMT_U8;
        break;
    case AV_SAMPLE_FMT_S16P:
        dst_fmt     =   AV_SAMPLE_FMT_S16;
        break;
    case AV_SAMPLE_FMT_S32P:
        dst_fmt     =   AV_SAMPLE_FMT_S32;
        break;    
    case AV_SAMPLE_FMT_FLTP:
        dst_fmt     =   AV_SAMPLE_FMT_FLT;
        break;
    case AV_SAMPLE_FMT_DBLP:
        dst_fmt     =   AV_SAMPLE_FMT_DBL;
        break;
    case AV_SAMPLE_FMT_S64P:
        dst_fmt     =   AV_SAMPLE_FMT_S64;
        break;
    }

    av_channel_layout_default( &out_layout, channel_layout );
    av_channel_layout_default( &in_layout, channel_layout );

    res     =   swr_alloc_set_opts2( &swr_ctx,
                                     &out_layout, dst_fmt,    sample_rate,      // output
                                     &in_layout,  sample_fmt, sample_rate,      // input
                                     0, nullptr );

    if( res < 0 )
        MYLOG( LOG::L_ERROR, "set swr opt fail. error code = %d", res );

    /*swr_ctx     =   swr_alloc_set_opts( swr_ctx,
                                        channel_layout, dst_fmt,    sample_rate,     // output
                                        channel_layout, sample_fmt, sample_rate,     // input 
                                        NULL, NULL );*/
    swr_init(swr_ctx);

    MYLOG( LOG::L_INFO, "audio sample format = %s", av_get_sample_fmt_name(sample_fmt) );
    MYLOG( LOG::L_INFO, "audio channel = %d, sample rate = %d", av_get_channel_layout_nb_channels(channel_layout), sample_rate );

    //
#ifdef FFMPEG_TEST
    //output_frame_func   =   std::bind( &VideoDecode::output_jpg_by_QT, this );
    output_frame_func   =   std::bind( &AudioDecode::output_pcm, this );
#endif

    Decode::init();
    return  R_SUCCESS;
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

算是測試用的函數, 輸出 pcm raw data 到檔案. 
********************************************************************************/
int     AudioDecode::output_pcm()
{
    static FILE *fp     =   fopen( output_pcm_path.c_str(), "wb+" );

    static constexpr int    out_channel =   2; // 目前預設輸出成兩聲道. 有空再改

    uint8_t     *data[2]    =   { 0 };  // S16 改 S32, 不確定是不是這邊的 array 要改成 4
                                        // int         byte_count     =   frame->nb_samples * 2 * 2;  // S16 改 S32, 改成 *4, 理論上資料量會增加, 但不確定是否改的是這邊
                                        // frame->nb_samples * 2 * 2     表示     分配樣本資料量 * 兩通道 * 每通道2位元組大小
    int         byte_count  =   av_samples_get_buffer_size( NULL, out_channel, frame->nb_samples, AV_SAMPLE_FMT_S16, 0 );

    unsigned char   *pcm    =   new uint8_t[byte_count];     

    if( pcm == nullptr )
        MYLOG( LOG::L_WARN, "pcm is null" );

    data[0]     =   pcm;    // 輸出格式為 AV_SAMPLE_FMT_S16(packet型別), 所以轉換後的 LR 兩通道都存在data[0]中
                            // 研究一下 S32 是不是存兩個資料
    int ret     =   swr_convert( swr_ctx,
                                 data, frame->nb_samples,                              //輸出 
                                 (const uint8_t**)frame->data, frame->nb_samples );    //輸入

    fwrite( pcm, 1, byte_count, fp );
    //MYLOG( LOG::L_DEBUG, "audio write %d. frame_count = %d", byte_count, frame_count );

    return  0;
}
#endif







/*******************************************************************************
AudioDecode::end()
********************************************************************************/
int     AudioDecode::end()
{
    if( frame_count > 0 )
    {
        MYLOG( LOG::L_INFO, "audio decode %d frames.", frame_count );
        int64_t     duration_time   =   1000LL * dec_ctx->frame_size * frame_count / dec_ctx->sample_rate; // ms
        int64_t     ms              =   duration_time % 1000;
        int64_t     sec             =   duration_time / 1000 % 60;
        int64_t     minute          =   duration_time / 1000 / 60 % 60;
        int64_t     hour            =   duration_time / 1000 / 60 / 60;
        MYLOG( LOG::L_INFO, "audio decode duration = [ %2lld : %2lld : %2lld . %3lld", hour, minute, sec, ms );
    }

    //
    if( swr_ctx != nullptr )
    {
        swr_close(swr_ctx);
        swr_ctx     =   nullptr;
    }
    
    sample_rate     =   0;
    channel_layout  =   0;
    sample_fmt      =   AV_SAMPLE_FMT_NONE;

    Decode::end();
    return  R_SUCCESS;
}





/*******************************************************************************
AudioDecode::output_audio_frame_info()
********************************************************************************/
void    AudioDecode::output_audio_frame_info()
{
    char    buf[AV_TS_MAX_STRING_SIZE]{0};
    int     per_sample  =   av_get_bytes_per_sample( static_cast<AVSampleFormat>(frame->format) );
    auto    pts_str     =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    MYLOG( LOG::L_INFO, "audio_frame n = %d, nb_samples = %d, pts : %s", frame_count, frame->nb_samples, pts_str );
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
AudioDecode::get_audeo_sample_size()
********************************************************************************/
int     AudioDecode::get_audeo_sample_size()
{
    switch( dec_ctx->sample_fmt )
    {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
        return  8;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        return  16;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        return  32;
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        return  32;
    case AV_SAMPLE_FMT_DBL:
    case AV_SAMPLE_FMT_DBLP:
        return  64;
    case AV_SAMPLE_FMT_S64:
    case AV_SAMPLE_FMT_S64P:
        return  64;
    default:
        MYLOG( LOG::L_ERROR, "un defined." );
    }
}



/*******************************************************************************
AudioDecode::get_audio_sample_type()
********************************************************************************/
int     AudioDecode::get_audio_sample_type()
{
    switch( dec_ctx->sample_fmt )
    {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
        return  2;   //   QAudioFormat  SampleType UnSignedInt
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
    case AV_SAMPLE_FMT_S64:
    case AV_SAMPLE_FMT_S64P:
        return  1;  // SignedInt
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_DBL:
    case AV_SAMPLE_FMT_DBLP:
        return  3;  // Float 
    default:
        MYLOG( LOG::L_ERROR, "un defined." );
    }
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
    //static constexpr int    out_channel =   2; // 目前預設輸出成兩聲道. 有空再改

    int     channel     =   get_audio_channel(),
            sample_rate =   get_audio_sample_rate(),
            sample_size =   get_audeo_sample_size();

    AVSampleFormat  sample_type =   dec_ctx->sample_fmt;

    int         byte_count  =   av_samples_get_buffer_size( NULL, channel, frame->nb_samples, sample_type, 0 );

    AudioData   ad;
    ad.pcm          =   nullptr;
    ad.bytes        =   0;
    ad.timestamp    =   0;

    ad.pcm    =   new uint8_t[byte_count];
    if( ad.pcm == nullptr )
        MYLOG( LOG::L_WARN, "pcm is null" );

    uint8_t *data[2]    =   { 0 };
    data[0] =   ad.pcm;    // 輸出格式為 AV_SAMPLE_FMT_S16(packet型別), 所以轉換後的 LR 兩通道都存在data[0]中

    int ret     =   swr_convert( swr_ctx,
                                 data, frame->nb_samples,                              //輸出 
                                 (const uint8_t**)frame->data, frame->nb_samples );    //輸入

    //memcpy( ad.pcm, frame->data, byte_count );

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
        MYLOG( LOG::L_INFO, "this stream has no audio stream" );
        return  R_SUCCESS;
    }

    //
    a_codec_id   =   fmt_ctx->streams[as_idx]->codecpar->codec_id;
    MYLOG( LOG::L_INFO, "code name = %s", avcodec_get_name(a_codec_id) );

    //
    channel     =   fmt_ctx->streams[as_idx]->codecpar->channels;
    sample_rate =   fmt_ctx->streams[as_idx]->codecpar->sample_rate;
    MYLOG( LOG::L_INFO, "channel = %d, sample rate = %d", channel, sample_rate );

    //
    double a_dur_ms = av_q2d( fmt_ctx->streams[as_idx]->time_base) * fmt_ctx->streams[as_idx]->duration;
    MYLOG( LOG::L_INFO, "frame duration = %lf ms", a_dur_ms );

#endif
    return  R_SUCCESS;
}
