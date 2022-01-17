#ifndef MUX_H
#define MUX_H

#include "maker_def.h"


struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVPacket;



class Mux
{
public:

    Mux();
    virtual ~Mux();

    Mux( const Mux& ) = delete;
    Mux( Mux&& ) = delete;

    Mux& operator = ( const Mux& ) = delete;
    Mux& operator = ( Mux&& ) = delete;

    void    open( EncodeSetting setting, AVCodecContext* v_ctx, AVCodecContext* a_ctx, AVCodecContext* s_ctx );
    bool    is_need_global_header();
    void    write_header();
    void    write_frame( AVPacket* pkt );

    virtual void    init( EncodeSetting setting );
    virtual void    write_end();
    virtual void    end();
    virtual bool    is_connect();   

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