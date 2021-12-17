#ifndef MUX_H
#define MUX_H

#include <stdint.h>

struct AVFormatContext;
struct AVOutputFormat;

struct OutputStream;
struct AVFrame;
struct AVCodec;
struct AVDictionary;
struct AVCodecContext;
struct AVStream;
struct AVPacket;

enum AVSampleFormat;
enum AVCodecID;



class Mux
{
public:
    Mux();
    ~Mux();

    Mux( const Mux& ) = delete;
    Mux( Mux&& ) = delete;

    Mux& operator = ( const Mux& ) = delete;
    Mux& operator = ( Mux&& ) = delete;

    void work();

private:

    void close_stream( AVFormatContext *oc, OutputStream *ost );
    int write_video_frame( AVFormatContext *oc, OutputStream *ost );
    int write_audio_frame( AVFormatContext *oc, OutputStream *ost );
    int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st, AVFrame *frame, AVPacket *pkt);


    // ���U�w�p�令�~���ǤJ
    AVFrame *get_video_frame( OutputStream *ost ); // �����|�令�q�~���ǤJ
    void fill_yuv_image( AVFrame *pict, int frame_index, int width, int height );
    AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
    void open_video( AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg );
    AVFrame *get_audio_frame(OutputStream *ost);
    void open_audio(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
    AVFrame *alloc_audio_frame( AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples );
    void add_stream(OutputStream *ost, AVFormatContext *oc, const AVCodec **codec, AVCodecID codec_id);

    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);



    AVFormatContext *output_ctx = nullptr;
    AVOutputFormat *output_fmt = nullptr;


};



#endif