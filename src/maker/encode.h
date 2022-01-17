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

    void    init( int st_idx, AVCodecID code_id, bool alloc_frame );
    bool    end_of_file();

    bool    is_flush();
    void    set_flush( bool flag );
    void    set_frame( AVFrame* _f );

    void        set_stream_time_base( AVRational _stb );
    AVRational  get_stream_time_base();

    virtual int     flush();


    virtual void    end();

    virtual void encode_timestamp();
    AVPacket*   get_pkt();

    AVCodec*    get_codec();

    AVCodecContext*     get_ctx();
    AVRational          get_timebase();
    AVFrame*            get_frame();

    // �ΨӤ�� encoder �i�h mux �����ᶶ�ǥ�.
    // video/audio �� subtitle ���ѼƤ��P.
    virtual AVRational  get_compare_timebase();


    virtual void unref_data();

    virtual int     send_frame();
    virtual int     recv_frame();
    //virtual void    unref_pkt();
    virtual bool    is_empty();

    virtual int64_t     get_pts() = 0;
    virtual void        next_frame() = 0;

protected:

    AVRational      stream_time_base { 0, 0 };

    AVCodec         *codec  =   nullptr;
    AVCodecContext  *ctx    =   nullptr;
    AVPacket        *pkt    =   nullptr;
    AVFrame         *frame  =   nullptr;

    //SwsContext      *sws_ctx    =   nullptr;

    int     frame_count     =   0;
    int     stream_index    =   -1;

    bool    flush_state     =   false;
    bool    eof_flag        =   false;

};


// �ΨӰ� pts ���, �M�w�֥��i�h mux.
bool    operator <= ( Encode& a, Encode& b );




#endif