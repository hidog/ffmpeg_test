#ifndef MUX_H
#define MUX_H

#include <stdint.h>
#include <functional>



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

    void init( AVCodecContext* v_ctx, AVCodec *v_codec , AVCodecContext* a_ctx, AVCodec *a_codec );
    void work();
    void write_header();
    void write_frame( AVPacket* pkt );
    void write_end();


    std::function<int64_t()> v_get_next_pts;
    std::function<int64_t()> a_get_next_pts;

    std::function<AVFrame*()> v_get_frame;
    std::function<AVFrame*()> a_get_frame;

    std::function<int(AVFrame*)> v_send_frame;
    std::function<int()> v_recv_frame;
    std::function<AVPacket*()> v_get_pkt;

    std::function<int(AVFrame*)> a_send_frame;
    std::function<int()> a_recv_frame;
    std::function<AVPacket*()> a_get_pkt;

//private:

    void close_stream( AVFormatContext *oc, OutputStream *ost );
    int write_video_frame( AVFormatContext *oc, OutputStream *ost );
    int write_audio_frame( AVFormatContext *oc, OutputStream *ost );
    int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st, AVFrame *frame, AVPacket *pkt);
    void add_stream( AVCodec *v_codec, AVCodec *a_codec );



    // 底下預計改成外部傳入
    AVFrame *get_video_frame( OutputStream *ost ); // 有機會改成從外部傳入
    void fill_yuv_image( AVFrame *pict, int frame_index, int width, int height );
    AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
    AVFrame *get_audio_frame(OutputStream *ost);
    AVFrame *alloc_audio_frame( AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples );






    AVFormatContext *output_ctx = nullptr;
    AVOutputFormat *output_fmt = nullptr;

    AVStream *v_stream = nullptr;
    AVStream *a_stream = nullptr;

};



#endif