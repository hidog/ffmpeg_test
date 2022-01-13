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


    virtual void    init( EncodeSetting setting );
    virtual void    open( EncodeSetting setting, AVCodecContext* v_ctx, AVCodecContext* a_ctx, AVCodecContext* s_ctx );
    virtual void    write_end();
    virtual void    end();    

    
    bool    is_need_global_header();
    void    write_header();
    void    write_frame( AVPacket* pkt );

    AVRational  get_video_stream_timebase();
    AVRational  get_audio_stream_timebase();
    AVRational  get_sub_stream_timebase();

protected:

    AVFormatContext     *output_ctx     =   nullptr;

    AVStream    *v_stream   =   nullptr;
    AVStream    *a_stream   =   nullptr;
    AVStream    *s_stream   =   nullptr;

};



#endif