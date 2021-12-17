#include "mux.h"



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "tool.h"

extern "C" {

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

}






/*******************************************************************************
Mux::Mux()
********************************************************************************/
Mux::Mux()
{}




/*******************************************************************************
Mux::Mux()
********************************************************************************/
Mux::~Mux()
{}





#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC


#if 1
// a wrapper around a single output AVStream
typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    AVPacket *tmp_pkt;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;
#endif




/*******************************************************************************
Mux::log_packet()
********************************************************************************/
void Mux::log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{}






/*******************************************************************************
Mux::write_frame()
********************************************************************************/
int Mux::write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st, AVFrame *frame, AVPacket *pkt)
{
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) 
    {
        fprintf(stderr, "Error sending a frame to the encoder: %d\n", ret);
        exit(1);
    }

    while( ret >= 0 ) 
    {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) 
        {
            fprintf(stderr, "Error encoding a frame: %d\n", ret);
            exit(1);
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(pkt, c->time_base, st->time_base);
        pkt->stream_index = st->index;

        /* Write the compressed frame to the media file. */
        log_packet(fmt_ctx, pkt);
        ret = av_interleaved_write_frame(fmt_ctx, pkt);
        /* pkt is now blank (av_interleaved_write_frame() takes ownership of
        * its contents and resets pkt), so that no unreferencing is necessary.
        * This would be different if one used av_write_frame(). */
        if (ret < 0) 
        {
            fprintf(stderr, "Error while writing output packet: %d\n", ret );
            exit(1);
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}








/*******************************************************************************
Mux::add_stream()
********************************************************************************/
void Mux::add_stream()
{
    AVCodecContext *c;
    int i;


    AVCodec *v_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodec *a_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);



    // video
    v_stream = avformat_new_stream( output_ctx, v_codec ); // 未來改成從encode傳入
    if( v_stream == nullptr )
        MYLOG( LOG::ERROR, "v_stream is nullptr." );
    v_stream->id = output_ctx->nb_streams - 1;
    v_stream->time_base.num = 1001; 
    v_stream->time_base.den = 24000;

    // audio
    a_stream = avformat_new_stream( output_ctx, a_codec );
    if( a_stream == nullptr )
        MYLOG( LOG::ERROR, "a_stream is nullptr." );
    a_stream->id = output_ctx->nb_streams - 1;
    a_stream->time_base.num = 1;
    a_stream->time_base.den = 48000;

    /* Some formats want stream headers to be separate. */
    // 未來看是否需要加入這段code
    //if( output_ctx->oformat->flags & AVFMT_GLOBALHEADER )
      //  c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}






/*******************************************************************************
Mux::alloc_audio_frame()
********************************************************************************/
AVFrame* Mux::alloc_audio_frame( AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) 
    {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) 
    {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) 
        {
            fprintf(stderr, "Error allocating an audio buffer\n");
            exit(1);
        }
    }

    return frame;
}







/*******************************************************************************
Mux::get_audio_frame()

 Prepare a 16 bit dummy audio frame of 'frame_size' samples and
'nb_channels' channels. 
********************************************************************************/
AVFrame* Mux::get_audio_frame(OutputStream *ost)
{
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];

    /* check if we want to generate more frames */
    AVRational avr{ 1, 1 };
    if (av_compare_ts(ost->next_pts, ost->enc->time_base, STREAM_DURATION, avr ) > 0)
        return NULL;

    for (j = 0; j <frame->nb_samples; j++) 
    {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->enc->channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    return frame;
}





/*******************************************************************************
Mux::write_audio_frame()

 encode one audio frame and send it to the muxer
 return 1 when encoding is finished, 0 otherwise
********************************************************************************/
int Mux::write_audio_frame( AVFormatContext *oc, OutputStream *ost )
{
    AVCodecContext *c;
    AVFrame *frame;
    int ret;
    int dst_nb_samples;

    c = ost->enc;

    frame = get_audio_frame(ost);

    if( frame != nullptr ) 
    {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples, c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
        * internally;
        * make sure we do not overwrite it here
        */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx, ost->frame->data, dst_nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) 
        {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }
        frame = ost->frame;

        AVRational avr{ 1, c->sample_rate };
        frame->pts = av_rescale_q(ost->samples_count, avr, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    return write_frame(oc, c, ost->st, frame, ost->tmp_pkt);
}






/*******************************************************************************
Mux::alloc_picture()
********************************************************************************/
AVFrame* Mux::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if( !picture )
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 0);
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "get buffer fail." );

    return picture;
}







/*******************************************************************************
Mux::fill_yuv_image()

Prepare a dummy image. 
********************************************************************************/
void Mux::fill_yuv_image(AVFrame *pict, int frame_index, int width, int height )
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) 
    {
        for (x = 0; x < width / 2; x++) 
        {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}





/*******************************************************************************
Mux::get_video_frame()
********************************************************************************/
AVFrame* Mux::get_video_frame( OutputStream *ost )
{
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames */
    AVRational avr{ 1, 1 };
    if( av_compare_ts( ost->next_pts, c->time_base, STREAM_DURATION, avr ) > 0 )
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
    * internally; make sure we do not overwrite it here */
    if( av_frame_make_writable(ost->frame) < 0 )
        exit(1);

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) 
    {
        /* as we only generate a YUV420P picture, we must convert it
        * to the codec pixel format if needed */
        if (!ost->sws_ctx) 
        {
            ost->sws_ctx = sws_getContext(c->width, c->height, AV_PIX_FMT_YUV420P, c->width, c->height, c->pix_fmt, SCALE_FLAGS, NULL, NULL, NULL);

            if (!ost->sws_ctx) 
            {
                fprintf(stderr, "Could not initialize the conversion context\n");
                exit(1);
            }
        }
        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
        sws_scale(ost->sws_ctx, (const uint8_t * const *) ost->tmp_frame->data,  ost->tmp_frame->linesize, 0, c->height, ost->frame->data, ost->frame->linesize);
    } 
    else 
    {
        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    }

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}






/*******************************************************************************
Mux::close_stream()

 encode one video frame and send it to the muxer
 return 1 when encoding is finished, 0 otherwise
********************************************************************************/
int Mux::write_video_frame( AVFormatContext *oc, OutputStream *ost )
{
    return write_frame( oc, ost->enc, ost->st, get_video_frame(ost), ost->tmp_pkt );
}







/*******************************************************************************
Mux::close_stream()
********************************************************************************/
void Mux::close_stream( AVFormatContext *oc, OutputStream *ost )
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    av_packet_free(&ost->tmp_pkt);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}




/*******************************************************************************
Mux::init()
********************************************************************************/
void Mux::init()
{
    //OutputStream video_st = { 0 }, audio_st = { 0 };
    const AVCodec *audio_codec = nullptr, *video_codec = nullptr;

    int ret;
    AVDictionary *opt = NULL;
    int i;

    //bool have_video = false, have_audio = false;
    //bool encode_video = false, encode_audio = false;

    /* allocate the output media context */
    avformat_alloc_output_context2( &output_ctx, NULL, NULL, "H:\\test.mp4" );
    //avformat_alloc_output_context2( &output_ctx, NULL, "mp4", "H:\\test.mp4" );
    if( output_ctx == nullptr ) 
        MYLOG( LOG::ERROR, "output_ctx = nullptr" );

    output_fmt = output_ctx->oformat;

    /*
    看改h265或其他編碼是不是要跟著修這個參數
    output_fmt->video_codec
    output_fmt->audio_codec
    */

    /* Add the audio and video streams using the default format codecs and initialize the codecs. */
    add_stream();

    av_dump_format( output_ctx, 0, "H:\\test.mp4", 1 );

    /* open the output file, if needed */
    if( !(output_fmt->flags & AVFMT_NOFILE) )
    {
        ret = avio_open( &output_ctx->pb, "H:\\test.mp4", AVIO_FLAG_WRITE );
        if( ret < 0 ) 
            MYLOG( LOG::ERROR, "open output file fail" );
    }
}






/*******************************************************************************
Mux::work()
********************************************************************************/
void Mux::work()
{
    int ret;
    AVDictionary *opt = nullptr;


    /* Write the stream header, if any. */
    ret = avformat_write_header( output_ctx, &opt );
    if (ret < 0) 
        MYLOG( LOG::ERROR, "write header fail. err = %d", ret );
//#define av_err2str(errnum) \
  //  av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)


    //while( encode_video == true || encode_audio == true ) 
    while(true)
    {
        /* select the stream to encode */
        /*ret = av_compare_ts( video_st.next_pts, video_st.enc->time_base, audio_st.next_pts, audio_st.enc->time_base );
        if (encode_video && (!encode_audio || ret <= 0) )        
            encode_video = !write_video_frame( output_ctx, &video_st );        
        else         
            encode_audio = !write_audio_frame( output_ctx, &audio_st );*/
    }

    /* Write the trailer, if any. The trailer must be written before you
    * close the CodecContexts open when you wrote the header; otherwise
    * av_write_trailer() may try to use memory that was freed on
    * av_codec_close(). */
    av_write_trailer(output_ctx);

    /* Close each codec. */
    /*if (have_video)
        close_stream( output_ctx, &video_st);
    if (have_audio)
        close_stream( output_ctx, &audio_st);*/

    if ( !(output_fmt->flags & AVFMT_NOFILE) )
        /* Close the output file. */
        avio_closep( &output_ctx->pb );

    /* free the stream */
    avformat_free_context(output_ctx);
}
