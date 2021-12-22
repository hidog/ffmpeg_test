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

    void    init( AVCodecContext* v_ctx, AVCodec *v_codec , AVCodecContext* a_ctx, AVCodec *a_codec );

    void    write_header();
    void    write_frame( AVPacket* pkt );
    void    write_end();

private:

    void close_stream();
    void add_stream( AVCodecContext* v_ctx, AVCodec *v_codec, AVCodecContext* a_ctx, AVCodec *a_codec );


    AVFormatContext *output_ctx = nullptr;
    AVOutputFormat *output_fmt = nullptr;

    AVStream *v_stream = nullptr;
    AVStream *a_stream = nullptr;

};



#endif