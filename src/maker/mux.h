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

    void    init( EncodeSetting setting );
    void    end();
    void    open( EncodeSetting setting, AVCodecContext* v_ctx, AVCodecContext* a_ctx, AVCodecContext* s_ctx );

    bool    is_need_global_header();

    void    write_header();
    void    write_frame( AVPacket* pkt );
    void    write_subtitle( AVPacket* pkt );
    void    write_end();

    AVRational  get_video_stream_timebase();
    AVRational  get_audio_stream_timebase();
    AVRational  get_sub_stream_timebase();

private:

    AVFormatContext     *output_ctx     =   nullptr;

    AVStream    *v_stream   =   nullptr;
    AVStream    *a_stream   =   nullptr;
    AVStream    *s_stream   =   nullptr;

};



#endif