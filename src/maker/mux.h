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

    void init();
    void work();

private:

    void close_stream( AVFormatContext *oc, OutputStream *ost );
    int write_video_frame( AVFormatContext *oc, OutputStream *ost );
    int write_audio_frame( AVFormatContext *oc, OutputStream *ost );
    int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st, AVFrame *frame, AVPacket *pkt);
    void add_stream();



    // 底下預計改成外部傳入
    AVFrame *get_video_frame( OutputStream *ost ); // 有機會改成從外部傳入
    void fill_yuv_image( AVFrame *pict, int frame_index, int width, int height );
    AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
    AVFrame *get_audio_frame(OutputStream *ost);
    AVFrame *alloc_audio_frame( AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples );



    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);



    AVFormatContext *output_ctx = nullptr;
    AVOutputFormat *output_fmt = nullptr;

    AVStream *v_stream = nullptr;
    AVStream *a_stream = nullptr;

};



#endif