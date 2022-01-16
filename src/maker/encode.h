#ifndef ENCODE_H
#define ENCODE_H



#include <stdint.h>

extern "C" {

#include <libavutil/rational.h>

} // end extern "C"


struct AVCodec;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct SwsContext;

enum AVCodecID;




class Encode
{
public:
    Encode();
    virtual ~Encode();

    Encode( const Encode& ) = delete;
    Encode( Encode&& ) = delete;

    Encode& operator = ( const Encode& ) = delete;
    Encode& operator = ( Encode&& ) = delete;

    void    init( int st_idx, AVCodecID code_id ); 

    bool    is_flush();
    void    set_flush( bool flag );
    int     flush();
    void    set_frame( AVFrame* _f );

    void        set_stream_time_base( AVRational _stb );
    AVRational  get_stream_time_base();

    virtual void    end();

    AVPacket*   get_pkt();
    AVCodec*    get_codec();

    AVCodecContext*     get_ctx();
    AVRational          get_timebase();

    virtual int     send_frame();
    virtual int     recv_frame();
    virtual void    unref_pkt();
    virtual bool    is_empty();

    virtual int64_t     get_pts() = 0;
    virtual AVFrame*    get_frame() = 0;

protected:

    AVRational      stream_time_base { 0, 0 };

    AVCodec         *codec  =   nullptr;
    AVCodecContext  *ctx    =   nullptr;
    AVPacket        *pkt    =   nullptr;
    AVFrame         *frame  =   nullptr;

    //SwsContext      *sws_ctx    =   nullptr;

    int     frame_count     =   0;
    int     stream_index    =   -1;

    bool    flush_state    =   false;

};


// 用來做 pts 比較, 決定誰先進去 mux.
bool    operator <= ( Encode& a, Encode& b );




#endif