#include "muxing.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <QImage>

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
#include <libavutil/imgutils.h>

}

#define STREAM_DURATION   50.0
//#define STREAM_FRAME_RATE 25 /* 25 images/s */
//#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

//#define SCALE_FLAGS SWS_BICUBIC



uint8_t  *video_dst_data[4]     =   { nullptr };
int      video_dst_linesize[4]  =   { 0 };
int      video_dst_bufsize      =   0;


AVStream* sub_stream = NULL;
AVPacket* sub_pkt = NULL;
AVSubtitle subtitle;






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

    struct AVFormatContext* sub_fmtctx;
    struct AVCodecContext*  sub_dec;
    int subidx;

} OutputStream;

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("pts : %d \n", pkt->stream_index );
}

static int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                       AVStream *st, AVFrame *frame, AVPacket *pkt)
{
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame to the encoder: %d\n", ret );
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding a frame: %d\n", ret );
            exit(1);
        }

       

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts( pkt, c->time_base, st->time_base);
        pkt->stream_index = st->index;

        /* Write the compressed frame to the media file. */
        log_packet(fmt_ctx, pkt);
        ret = av_interleaved_write_frame(fmt_ctx, pkt);
        /* pkt is now blank (av_interleaved_write_frame() takes ownership of
         * its contents and resets pkt), so that no unreferencing is necessary.
         * This would be different if one used av_write_frame(). */
        if (ret < 0) {
            fprintf(stderr, "Error while writing output packet: %d\n", ret );
            exit(1);
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}

/* Add an output stream. */
static void add_stream( OutputStream *ost, AVFormatContext *oc, const AVCodec **codec, enum AVCodecID codec_id )
{
    int ret = 0;
    AVCodec* sub_codec = NULL;

    AVCodecContext *c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) 
    {
        fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        exit(1);
    }

    ost->tmp_pkt = av_packet_alloc();
    if (!ost->tmp_pkt) 
    {
        fprintf(stderr, "Could not allocate AVPacket\n");
        exit(1);
    }

    ost->st = avformat_new_stream( oc, NULL );
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type) 
    {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = AV_SAMPLE_FMT_FLTP; //AV_SAMPLE_FMT_S16P;
        c->bit_rate    = 320000;
        c->sample_rate = 48000;
        /*if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }*/
        //c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        /*if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }*/
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        //ost->st->time_base = (AVRational){ 1, c->sample_rate };
        ost->st->time_base.num = 1;
        ost->st->time_base.den = c->sample_rate;

        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = 80000000;
        /* Resolution must be a multiple of two. */
        c->width    = 1920;
        c->height   = 1080;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        //ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        ost->st->time_base.num = 1001; //1;
        ost->st->time_base.den = 24000; //STREAM_FRAME_RATE;

        c->time_base       = ost->st->time_base;

        c->gop_size      = 1;//30; /* emit one intra frame every twelve frames at most */
        c->max_b_frames  = 0;//15;

        c->pix_fmt       = AV_PIX_FMT_YUV420P;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B-frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
        break;

    case AVMEDIA_TYPE_SUBTITLE :

        //ost->st->codecpar->codec_tag = 0x6134706d;
        //ost->st->codecpar->codec_type    =   AVMEDIA_TYPE_SUBTITLE;
        //ost->st->codecpar->codec_id      =   av_guess_codec( oc->oformat, nullptr, oc->url, nullptr, ost->st->codecpar->codec_type );

        ost->sub_fmtctx = avformat_alloc_context();
        ret = avformat_open_input( &ost->sub_fmtctx, "J:\\test.ass", nullptr, nullptr );
        ret = avformat_find_stream_info( ost->sub_fmtctx, nullptr );
        ost->subidx = av_find_best_stream( ost->sub_fmtctx, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0 );
        
        sub_stream = ost->sub_fmtctx->streams[ost->subidx];

        sub_codec = avcodec_find_decoder( sub_stream->codecpar->codec_id );
        ost->sub_dec = avcodec_alloc_context3( sub_codec );

        ret = avcodec_parameters_to_context( ost->sub_dec, sub_stream->codecpar );
        if( ret < 0 )
            printf(" add_stream AVMEDIA_TYPE_SUBTITLE fail.\n" );

        ost->sub_dec->pkt_timebase = sub_stream->time_base;

        // AV_TIME_BASE
        //uint8_t *subtitle_header;
        //int subtitle_header_size;
        c->pkt_timebase = AVRational{ 1, 1000 }; // �ݱԭz�o�䤣�v�T���G, ���Ҥ@�U���X�v�����ɭԬO�_�ݭn�ק�
        //c->pkt_timebase = AVRational{ 1, AV_TIME_BASE }; // �ݱԭz�o�䤣�v�T���G, ���Ҥ@�U���X�v�����ɭԬO�_�ݭn�ק�
        c->time_base = AVRational{ 1, 1000 };
        ost->st->time_base = c->time_base;
        


        //ret = av_opt_set( c, "scale", "1920x1080", 1 );




        //ret = av_opt_set_image_size( c->priv_data, "video_size", 1920, 1080, 0 );



        ret = avcodec_open2( ost->sub_dec, sub_codec, nullptr );
        if( ret < 0 )
            printf("AVMEDIA_TYPE_SUBTITLE avcodec_open2 fail\n");

        if( ost->sub_dec->subtitle_header != NULL )
        {
            // if not set header, open subtitle enc will fail.
            c->subtitle_header = (uint8_t*)av_mallocz( ost->sub_dec->subtitle_header_size + 1 );   // �S�d�� + 1 ���z��
            memcpy( c->subtitle_header, ost->sub_dec->subtitle_header, ost->sub_dec->subtitle_header_size );
            c->subtitle_header_size = ost->sub_dec->subtitle_header_size;


            printf("%s\n", c->subtitle_header );


            //ret = av_opt_set( c->subtitle_header, "scale", "1920x1080", 1 );
        }

        break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}












/**************************************************************/
/* audio output */

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            exit(1);
        }
    }

    return frame;
}



static void open_subtitle(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int ret;
    AVDictionary *opt = NULL;


    c = ost->enc;



    //av_dict_set( &opt, "scale", "1920x1080", 0);


    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2( c, codec, &opt );
    av_dict_free(&opt);
    if( ret < 0 )
        printf("open_subtitle open fail.");

    ret = avcodec_parameters_from_context( ost->st->codecpar, c );
    //ost->st->codecpar->codec_tag = 0x6134706d;

    if( ret < 0 )
        printf("open_subtitle copy param fail." );

    if( ost->st->duration <= 0 && sub_stream->duration > 0 )
        ost->st->duration = av_rescale_q( sub_stream->duration, sub_stream->time_base, ost->st->time_base );

}



static void open_audio(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;

    c = ost->enc;

    /* open it */
    av_dict_copy( &opt, opt_arg, 0 );
    ret = avcodec_open2( c, codec, &opt );
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec: %d\n", ret );
        exit(1);
    }

    /* init signal generator */
    ost->t     = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                       c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       48000, nb_samples);
    //ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
      //                                 c->sample_rate, nb_samples);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }

    /* create resampler context */
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        exit(1);
    }

    /* set options */
    av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      c->sample_fmt,     0);
    av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        exit(1);
    }
}




/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
static AVFrame *get_audio_frame(OutputStream *ost)
{
#if 0
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];

    /* check if we want to generate more frames */
    AVRational avr{1,1};
    if (av_compare_ts(ost->next_pts, ost->enc->time_base, STREAM_DURATION, avr ) > 0)
        return NULL;

    for (j = 0; j <frame->nb_samples; j++) {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->enc->channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    return frame;
#else

    static int a_frame_count = 0;

    AVFrame *frame = ost->frame;
    int j, i, v;
    //int16_t *q = (int16_t*)frame->data[0];

    /* check if we want to generate more frames */
    //AVRational avr{1,1};
    //if (av_compare_ts(ost->next_pts, ost->enc->time_base, STREAM_DURATION, avr ) > 0)
    //if( a_frame_count > 800 )
      //  return NULL;


    static FILE    *fp     =   fopen( "J:\\test.pcm", "rb" );
    int     ret;
    int16_t     intens[2];

    //if( feof(fp) != 0 )
    if( a_frame_count > 4000 )
        return NULL;

    ret = av_frame_make_writable(frame);
    if( ret < 0 )
        printf( "frame not writeable.\n" );
    
    for( i = 0; i < frame->nb_samples; i++ )
    {
        ret = fread( intens, 2, sizeof(int16_t), fp );
        if( ret == 0 )
        {
            intens[0] = 0;
            intens[1] = 0;
        }
    
        // �h�n�D�o��ݭn�t�~�B�z
        if( ost->enc->codec_id == AV_CODEC_ID_MP3 )
        {
            *((int16_t*)(frame->data[0]) + i)   =   intens[0];
            *((int16_t*)(frame->data[1]) + i)   =   intens[1];
        }
        else
        {
            *((float*)(frame->data[0]) + i)   =   1.0 * intens[0] / INT16_MAX;
            *((float*)(frame->data[1]) + i)   =   1.0 * intens[1] / INT16_MAX;
        }
        
    }


    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    a_frame_count++;

    return frame;

#endif
}





/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
static int write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    AVFrame *frame;
    int ret;
    int dst_nb_samples;

    c = ost->enc;

    frame = get_audio_frame(ost);

    if (frame) 
    {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
#if 0
        dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                        c->sample_rate, c->sample_rate, AV_ROUND_UP);
        //av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }
        //frame = ost->frame;
#endif



        AVRational avr { 1, c->sample_rate };
        frame->pts = av_rescale_q(ost->samples_count, avr, c->time_base);
        ost->samples_count += frame->nb_samples; //  dst_nb_samples;
    }

    return write_frame(oc, c, ost->st, frame, ost->tmp_pkt);
}







static int write_subtitle_frame( AVFormatContext *oc, OutputStream *ost )
{
    static const int subtitle_out_max_size = 1024*1024;

    AVCodecContext *c;
    //AVFrame *frame;
    int ret, got_sub;


    int dst_nb_samples;

    c = ost->enc;

    if( sub_pkt->size > 0 )
    {
        printf( "sub_pkt = %s\n", sub_pkt->data );


        memset( &subtitle, 0, sizeof(subtitle) );            
        ret = avcodec_decode_subtitle2( ost->sub_dec, &subtitle, &got_sub, sub_pkt ); 

        printf( "subtitle = %s\n", subtitle.rects[0]->ass );


        if( ret < 0 )
            printf( "write_subtitle_frame decode fail.\n" );
        if( got_sub > 0 )
        {
            subtitle.pts                   =    av_rescale_q( sub_pkt->pts, ost->sub_dec->pkt_timebase, AVRational{ 1, 1000 } );
            //subtitle.start_display_time    =    subtitle.pts;
            //subtitle.end_display_time      +=    subtitle.start_display_time;
            int64_t sub_duration           =    subtitle.end_display_time - subtitle.start_display_time;
            sub_duration /= 1000;

            uint8_t*    subtitle_out        =   (uint8_t*)av_mallocz(subtitle_out_max_size);
            int         subtitle_out_size   =   avcodec_encode_subtitle( c , subtitle_out, subtitle_out_max_size, &subtitle );

            //printf( "%s\n", subtitle.rects[0]->ass );

            AVPacket    pkt;
            av_init_packet( &pkt );

            if( subtitle_out_size == 0 )
                pkt.data    =   nullptr;
            else
                pkt.data    =   subtitle_out;
           
            printf( "pkt = %s\n", pkt.data );

            pkt.size        =   subtitle_out_size;  //strlen( (char*)pkt.data );
            pkt.pts         =   av_rescale_q( subtitle.pts, AVRational { 1, AV_TIME_BASE }, ost->st->time_base );
            pkt.duration    =   av_rescale_q( sub_duration, ost->sub_dec->pkt_timebase, ost->st->time_base );
            pkt.dts         =   pkt.pts;

            static int64_t  last_pts    =   pkt.pts;    // �ΨӳB�z flush �� pts

                                                        // note: flush ���B�z�i�H�Ѧ� got_sub �Ψ�L�覡.
            if( pkt.pts > 0 )
                last_pts    =   pkt.pts;
            else
            {
                pkt.pts =   last_pts;
                pkt.dts =   last_pts;
                pkt.duration    =   0;
            }

            //AVRational avr { 1, 1000 };
            //pkt.pts = av_rescale_q( ost->samples_count, avr, c->time_base);


            int ret =   0;
            pkt.stream_index = ost->st->index;
            if( subtitle_out_size != 0 )
                ret =   av_interleaved_write_frame( oc, &pkt );  // �S����������...
                //ret =   av_write_frame( oc, &pkt );
            else 
                ret =   av_interleaved_write_frame( oc, &pkt );  // �S����������...

            if( ret < 0 )
                printf( "write_subtitle_frame write fail." );

            av_free( subtitle_out );

            return 0;
        }

        avsubtitle_free(&subtitle);
    }
    else
        printf("write_subtitle_frame size < 0. error.\n");


    /*if (frame) 
    {
        AVRational avr { 1, c->sample_rate };
        frame->pts = av_rescale_q(ost->samples_count, avr, c->time_base);
        ost->samples_count += frame->nb_samples; //  dst_nb_samples;
    }

    return write_frame(oc, c, ost->st, frame, ost->tmp_pkt);*/

    return 1;
}





















/**************************************************************/
/* video output */

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}




static void open_video(AVFormatContext *oc, const AVCodec *codec,
                       OutputStream *ost, AVDictionary *opt_arg)
{
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %d\n", ret );
        exit(1);
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }
}






/* Prepare a dummy image. */
static void fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}




static AVFrame *get_video_frame(OutputStream *ost)
{
#if 0
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames */
    AVRational avr { 1, 1 };
    if (av_compare_ts(ost->next_pts, c->time_base, STREAM_DURATION, avr ) > 0)
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        /* as we only generate a YUV420P picture, we must convert it
         * to the codec pixel format if needed */
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          AV_PIX_FMT_YUV420P,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                fprintf(stderr,
                        "Could not initialize the conversion context\n");
                exit(1);
            }
        }
        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
        sws_scale(ost->sws_ctx, (const uint8_t * const *) ost->tmp_frame->data,
                  ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
                  ost->frame->linesize);
    } else {
        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    }

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
#else
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames */
    /*AVRational avr { 1, 1 };
    if (av_compare_ts(ost->next_pts, c->time_base, STREAM_DURATION, avr ) > 0)
        return NULL;*/

    static int v_frame_count = 0;
    //if( v_frame_count > 35719 )
    if( v_frame_count > 3000 )
        return NULL;


    /* when we pass a frame to the encoder, it may keep a reference to it
    * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);

    char str[1000];


    if( !ost->sws_ctx )
    {
        ost->sws_ctx = sws_getContext( 1920, 1080, AV_PIX_FMT_BGRA,
                                       1920, 1080, AV_PIX_FMT_YUV420P,
                                       SWS_BICUBIC, NULL, NULL, NULL);
        if (!ost->sws_ctx) 
        {
            fprintf(stderr, "Could not initialize the conversion context\n");
            exit(1);
        }
    }

    sprintf( str, "J:\\jpg\\%d.jpg", v_frame_count );
    printf("file = %s\n", str );
    QImage img(str);

    int linesize[8] = { img.bytesPerLine() };
    uint8_t* ptr[4] = { img.bits() };

    sws_scale( ost->sws_ctx, ptr, linesize, 0, 1080, video_dst_data, video_dst_linesize );

    memcpy( ost->frame->data[0], video_dst_data[0], video_dst_linesize[0]*1080 );
    memcpy( ost->frame->data[1], video_dst_data[1], video_dst_linesize[1]*1080/2 );
    memcpy( ost->frame->data[2], video_dst_data[2], video_dst_linesize[2]*1080/2 );

    ost->frame->pts = ost->next_pts++;

    v_frame_count++;

    return ost->frame;

#endif
}






/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
static int write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
    return write_frame(oc, ost->enc, ost->st, get_video_frame(ost), ost->tmp_pkt);
}

static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    av_packet_free(&ost->tmp_pkt);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}







/*typedef struct AVIOInterruptCB {
    int (*callback)(void*);
    void *opaque;
} AVIOInterruptCB;*/



struct AVIOInterruptCB cb;


int test( void* ptr )
{
    printf("test");
    return 0;
}

/**************************************************************/
/* media file output */

int muxing()
{

    cb.opaque = NULL;
    cb.callback = &test;


    video_dst_bufsize   =   av_image_alloc( video_dst_data, video_dst_linesize, 1920, 1080, AV_PIX_FMT_YUV420P, 1 );





    OutputStream video_st = { 0 }, audio_st = { 0 }, subtitle_st = {0};
    AVOutputFormat *fmt;
    const char *filename;
    AVFormatContext *oc;
    const AVCodec *audio_codec = NULL, *video_codec = NULL, *subtitle_codec = NULL;
    int ret;
    int have_video = 0, have_audio = 0, have_subtitle = 0;
    int encode_video = 0, encode_audio = 0, encode_subtitle = 0;
    AVDictionary *opt = NULL;
    int i;



    filename = "J:\\output.mkv";


    /* allocate the output media context */
    avformat_alloc_output_context2( &oc, NULL, NULL, filename );
    if (!oc) 
    {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc)
        return 1;

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE) 
    {
        fmt->video_codec = AV_CODEC_ID_H264;
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) 
    {
        fmt->audio_codec = AV_CODEC_ID_AAC;
        add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
    }
    //if (fmt->subtitle_codec != AV_CODEC_ID_NONE) 
#if 1
    {
        // AV_CODEC_ID_MOV_TEXT
        //fmt->subtitle_codec = AV_CODEC_ID_SUBRIP;
        fmt->subtitle_codec = AV_CODEC_ID_ASS;
        add_stream( &subtitle_st, oc, &subtitle_codec, fmt->subtitle_codec );
        have_subtitle = 1;
        encode_subtitle = 1;
    }
#endif



    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);

    if (have_audio)
        open_audio(oc, audio_codec, &audio_st, opt);

    if( have_subtitle)
        open_subtitle( oc, subtitle_codec, &subtitle_st, opt );

    av_dump_format(oc, 0, filename, 1);


    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) 
    {
        //ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        oc->interrupt_callback = cb;
        ret     =   avio_open2( &oc->pb, filename, AVIO_FLAG_WRITE, &oc->interrupt_callback, nullptr );

        if (ret < 0) 
        {
            fprintf(stderr, "Could not open '%s': %d\n", filename, ret);
            return 1;
        }
    }



    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) 
    {
        //av_err2str(ret)
        char temp_str[AV_ERROR_MAX_STRING_SIZE];
        

        fprintf(stderr, "Error occurred when opening output file: %d %s\n", ret, av_make_error_string( temp_str, AV_ERROR_MAX_STRING_SIZE, ret) );
        return 1;
    }



    int cpr1, cpr2;  // cpr = compare



    sub_pkt = av_packet_alloc();
    ret = av_read_frame( subtitle_st.sub_fmtctx, sub_pkt );


    int encode_target = 0; // 0 video   1 audio    2 subtitle


    int video_cc = 0, audio_cc = 0, sub_cc = 0;


    //sub_pkt->pts = 99999999;


    while( encode_video || encode_audio || encode_subtitle )
    {
        //cpr2 = -1;
        //printf( "v time base = %d %d\n", video_st.st->time_base.num, video_st.st->time_base.den );

        printf( "vc = %d, ac = %d, sc = %d\n", video_cc, audio_cc, sub_cc );

        cpr1 = av_compare_ts( video_st.next_pts, video_st.enc->time_base, audio_st.next_pts, audio_st.enc->time_base );
        // cpr <= 0  video
        // cpr > 0  audio
        if( cpr1 <= 0 )        
            cpr2 = av_compare_ts( video_st.next_pts, video_st.enc->time_base, sub_pkt->pts , subtitle_st.enc->time_base );        
        else
            cpr2 = av_compare_ts( audio_st.next_pts, audio_st.enc->time_base, sub_pkt->pts , subtitle_st.enc->time_base );
        // cpr2 > 0    subtitle
        // cpr2 <= 0   cpr1 <= 0    video
        // cpr2 <= 0   cpr1 > 0     audio
        if( cpr2 > 0 )
        {
            encode_target = 2;
            sub_cc++;
        }
        else if( cpr1 <= 0 )
        {
            encode_target = 0;
            video_cc++;
        }
        else
        {
            encode_target = 1;
            audio_cc++;
        }
        
        if( encode_target == 0 && encode_video == 0 )
        {
            if( encode_audio != 0 )
                encode_target = 1;
            else 
                encode_target = 2;
        }
        else if( encode_target == 1 && encode_audio == 0 )
        {
            if( encode_video != 0 )
                encode_target = 0;
            else 
                encode_target = 2;
        }
        else if( encode_target == 2 && encode_subtitle == 0 )
        {
            if( encode_video != 0 )
                encode_target = 0;
            else 
                encode_target = 1;
        }


        /* select the stream to encode */
        if( encode_target == 0 )         
            encode_video = !write_video_frame(oc, &video_st);        
        else if( encode_target == 1 )       
            encode_audio = !write_audio_frame(oc, &audio_st);
        else
        {
            // encode subtitle
            encode_subtitle = !write_subtitle_frame( oc, &subtitle_st );
            ret = av_read_frame( subtitle_st.sub_fmtctx, sub_pkt );
        }
        
    }

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    if (have_video)
        close_stream(oc, &video_st);
    if (have_audio)
        close_stream(oc, &audio_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);

    return 0;
}
