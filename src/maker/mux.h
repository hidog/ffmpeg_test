#ifndef MUX_H
#define MUX_H


#include "maker_def.h"


extern "C" {

#include <libavutil/rational.h>

} // end extern "C"




struct AVFormatContext;
struct AVCodec;
struct AVCodecContext;
struct AVStream;
struct AVPacket;


class Mux
{
public:

    Mux();
    ~Mux();

    Mux( const Mux& ) = delete;
    Mux( Mux&& ) = delete;

    Mux& operator = ( const Mux& ) = delete;
    Mux& operator = ( Mux&& ) = delete;

    void    init( EncodeSetting setting, AVCodecContext* v_ctx, AVCodec *v_codec , AVCodecContext* a_ctx, AVCodec *a_codec );
    void    end();

    void    write_header();
    void    write_frame( AVPacket* pkt );
    void    write_end();

    AVRational  get_video_stream_timebase();
    AVRational  get_audio_stream_timebase();

private:

    void    add_stream( AVCodecContext* v_ctx, AVCodec *v_codec, AVCodecContext* a_ctx, AVCodec *a_codec );

    AVFormatContext     *output_ctx     =   nullptr;

    AVStream    *v_stream   =   nullptr;
    AVStream    *a_stream   =   nullptr;

};



#endif