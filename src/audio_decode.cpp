﻿#include "audio_decode.h"
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
{
    AVMediaType   type  =   AVMEDIA_TYPE_AUDIO; 
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
{}





/*******************************************************************************
AudioDecode::open_codec_context()
********************************************************************************/
int     AudioDecode::open_codec_context( int stream_index, AVFormatContext *fmt_ctx )
{
    Decode::open_codec_context( stream_index, fmt_ctx, type );
    //dec_ctx->thread_count = 4;
    return  SUCCESS;
}





/*******************************************************************************
AudioDecode::init()
********************************************************************************/
int     AudioDecode::init()
{
    sample_rate     =   dec_ctx->sample_rate;
    sample_fmt      =   dec_ctx->sample_fmt;
    channel_layout  =   dec_ctx->channel_layout;  //dec_ctx->channels;

    //assert( dec_ctx->sample_fmt == AV_SAMPLE_FMT_FLTP ); // 如果遇到不同的  在看是不是要調整audio output的sample size



    // 有些影片要用這個來初始化swr_ctx, 研究一下這個問題
    int aaa = av_get_default_channel_layout( dec_ctx->channels );

    //int aaa = av_get_channel_layout_nb_channels( dec_ctx->channel_layout );
    MYLOG( LOG::DEBUG, "aaa = %d", aaa );

    // 試著想要改變 sample rate, 但沒成功.                                                  
    // S16 改 S32, 需要修改pcm的部分. 需要找時間研究
    swr_ctx     =   swr_alloc_set_opts( swr_ctx,
                                        av_get_default_channel_layout(2), AV_SAMPLE_FMT_S16, sample_rate,           // output
                                        //channel_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate,         // input  有些case要用底下的做初始化,研究正確用法
                                        aaa, dec_ctx->sample_fmt, dec_ctx->sample_rate,         // input
                                        NULL, NULL );
    swr_init(swr_ctx);

    MYLOG( LOG::INFO, "audio sample format = %s", av_get_sample_fmt_name(sample_fmt) );
    MYLOG( LOG::INFO, "audio channel = %d, sample rate = %d", channel_layout, sample_rate );

    Decode::init();
    return  SUCCESS;
}





/*******************************************************************************
AudioDecode::end()
********************************************************************************/
int     AudioDecode::end()
{
    swr_close(swr_ctx);

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
    MYLOG( LOG::INFO, "audio_frame n = %d, nb_samples = %d, pts : %s", frame_count++, frame->nb_samples, pts_str );
}





/*******************************************************************************
AudioDecode::output_audio_data()
********************************************************************************/
AudioData   AudioDecode::output_audio_data()
{
    AudioData   ad { nullptr, 0 };

    // 有空來修改這邊 要能動態根據 mp4 檔案做調整

    uint8_t     *data[2]    =   { 0 };  // S16 改 S32, 不確定是不是這邊的 array 要改成 4
    int         byte_count     =   frame->nb_samples * 2 * 2;  // S16 改 S32, 改成 *4, 理論上資料量會增加, 但不確定是否改的是這邊

    unsigned char   *pcm    =   new uint8_t[byte_count];     // frame->nb_samples * 2 * 2     表示     分配樣本資料量 * 兩通道 * 每通道2位元組大小

    if( pcm == nullptr )
        MYLOG( LOG::WARN, "pcm is null" );

    data[0]     =   pcm;    // 輸出格式為AV_SAMPLE_FMT_S16(packet型別),所以轉換後的 LR 兩通道都存在data[0]中
    int ret     =   swr_convert( swr_ctx,
                                 //data, sample_rate,                                    //輸出
                                 data, frame->nb_samples,                              //輸出   遇到資料過長超過buffer限制的時候出錯
                                 (const uint8_t**)frame->data, frame->nb_samples );    //輸入

    ad.pcm      =   pcm;
    ad.bytes    =   byte_count;

    return ad;
}





// https://stackoverflow.com/questions/16904841/how-to-encode-resampled-pcm-audio-to-aac-using-ffmpeg-api-when-input-pcm-samples
// put frame data into buffer of fixed size
#if 0
bool ffmpegHelper::putAudioBuffer(const AVFrame *pAvFrameIn, AVFrame **pAvFrameBuffer, AVCodecContext *dec_ctx, int frame_size, int &k0) 
{
    // prepare pFrameAudio
    if (!(*pAvFrameBuffer)) 
    {
        if (!(*pAvFrameBuffer = av_frame_alloc())) 
        {
            av_log(NULL, AV_LOG_ERROR, "Alloc frame failed\n");
            return false;
        } 
        else 
        {
            (*pAvFrameBuffer)->format = dec_ctx->sample_fmt;
            (*pAvFrameBuffer)->channels = dec_ctx->channels;
            (*pAvFrameBuffer)->sample_rate = dec_ctx->sample_rate;
            (*pAvFrameBuffer)->nb_samples = frame_size;
            int ret = av_frame_get_buffer(*pAvFrameBuffer, 0);
            if (ret < 0) 
            {
                char err[500];
                av_log(NULL, AV_LOG_ERROR, "get audio buffer failed: %s\n",
                                            av_make_error_string(err, AV_ERROR_MAX_STRING_SIZE, ret));
                return false;
            }
            (*pAvFrameBuffer)->nb_samples = 0;
            (*pAvFrameBuffer)->pts = pAvFrameIn->pts;
        }
    }

    // copy input data to buffer
    int n_channels = pAvFrameIn->channels;
    int new_samples = min(pAvFrameIn->nb_samples - k0, frame_size - (*pAvFrameBuffer)->nb_samples);
    int k1 = (*pAvFrameBuffer)->nb_samples;

    if (pAvFrameIn->format == AV_SAMPLE_FMT_S16) 
    {
        int16_t *d_in = (int16_t *)pAvFrameIn->data[0];
        d_in += n_channels * k0;
        int16_t *d_out = (int16_t *)(*pAvFrameBuffer)->data[0];
        d_out += n_channels * k1;

        for (int i = 0; i < new_samples; ++i) 
        {
            for (int j = 0; j < pAvFrameIn->channels; ++j) 
            {
                *d_out++ = *d_in++;
            }
        }
    } 
    else 
    {
        printf("not handled format for audio buffer\n");
        return false;
    }

    (*pAvFrameBuffer)->nb_samples += new_samples;
    k0 += new_samples;

    return true;
}


// transcoding needed
int got_frame;
AVMediaType stream_type;
// decode the packet (do it your self)
decodePacket(packet, dec_ctx, &pAvFrame_, got_frame);

if (enc_ctx->codec_type == AVMEDIA_TYPE_AUDIO) 
{
    ret = 0;
    // break audio packet down to buffer
    if (enc_ctx->frame_size > 0) 
    {
        int k = 0;
        while (k < pAvFrame_->nb_samples) 
        {
            if (!putAudioBuffer(pAvFrame_, &pFrameAudio_, dec_ctx, enc_ctx->frame_size, k))
                return false;
            if (pFrameAudio_->nb_samples == enc_ctx->frame_size) 
            {
                // the buffer is full, encode it (do it yourself)
                ret = encodeFrame(pFrameAudio_, stream_index, got_frame, false);
                if (ret < 0)
                    return false;
                pFrameAudio_->pts += enc_ctx->frame_size;
                pFrameAudio_->nb_samples = 0;
            }
        }
    } 
    else 
    {
        ret = encodeFrame(pAvFrame_, stream_index, got_frame, false);
    }
} 
else 
{
    // encode packet directly
    ret = encodeFrame(pAvFrame_, stream_index, got_frame, false);
}


uint8_t *buffer = (uint8_t*) malloc(1024);
AVFrame *frame = av_frame_alloc();
while((fread(buffer, 1024, 1, fp)) == 1) 
{
    frame->data[0] = buffer;
}
#endif