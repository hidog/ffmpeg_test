#ifndef DECODE_H
#define DECODE_H

#include <functional>
#include "tool.h"


struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVCodec;
struct AVStream;
enum   AVMediaType;


/*
ffmpeg 範例 demuxing_decoding 有播放 raw data 的指令.
*/


class DLL_API Decode
{
public:

    Decode( AVMediaType _type );
    virtual ~Decode();

    Decode( const Decode& ) = delete;
    Decode( Decode&& ) = delete;

    Decode& operator = ( const Decode& ) = delete;
    Decode& operator = ( Decode&& ) = delete;
    
    virtual void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) = 0;
    virtual int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx, AVMediaType type );

    virtual int     init();
    virtual int     end();  
    virtual void    flush_for_seek();
    
    virtual AVFrame*    get_frame();

    virtual int     send_packet( AVPacket *pkt );
    virtual int     recv_frame( int index );
    virtual void    unref_frame();

    int     get_frame_count();
    void    set_is_current( bool flag );
    bool    get_is_current();

    AVMediaType     get_decode_context_type();
    AVCodecContext* get_decode_context();
    AVStream*       get_stream();

#ifdef FFMPEG_TEST
    virtual int     flush();
    std::function<int()>    output_frame_func;
#endif

protected:
    AVMediaType     type;

    // 有空改成 private + protected interface.
    AVCodecContext  *dec_ctx    =   nullptr;    
    AVStream        *stream     =   nullptr;
    AVFrame         *frame      =   nullptr;
   
    int     frame_count =   -1;
    bool    is_current  =   false;

private:

};


#endif