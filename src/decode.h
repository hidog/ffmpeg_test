#ifndef DECODE_H
#define DECODE_H

#include <string>
#include <functional>
#include "tool.h"



struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVCodec;
struct AVStream;
enum   AVMediaType;

//template class __declspec( dllexport ) std::function<int()>; // fix compiler warning. 主要是匯出成dll的話需要顯式具現化樣板



class DLL_API Decode
{
public:

    Decode();
    virtual ~Decode();

    Decode( const Decode& ) = delete;
    Decode( Decode&& ) = delete;

    Decode& operator = ( const Decode& ) = delete;
    Decode& operator = ( Decode&& ) = delete;

    //
    virtual int     init();
    virtual int     end();

    //
    virtual void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) = 0;
    virtual int     open_codec_context( AVFormatContext *fmt_ctx ) = 0;
    virtual bool    exist_stream();

    int     send_packet( const AVPacket *pkt );
    int     recv_frame( int index );
    void    unref_frame();
    int     get_frame_count();
    bool    is_index( int index );
    int     current_index();


    //
    AVFrame*        get_frame();
    AVMediaType     get_decode_context_type();
    AVCodecContext* get_decode_context();

#ifdef FFMPEG_TEST
    int     flush();
    std::function<int()>    output_frame_func;
#endif



protected:

    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx, AVMediaType type );
    int     open_all_codec( AVFormatContext *fmt_ctx, AVMediaType type );


    std::map<int,AVCodecContext*>   dec_map;
    std::map<int,AVStream*>         stream_map;

    int cs_index    =   -1;     // current stream index.

    AVCodecContext  *dec_ctx    =   nullptr;
    AVStream        *stream     =   nullptr;
    AVFrame         *frame      =   nullptr;

    int     frame_count =   0;

private:

};


#endif