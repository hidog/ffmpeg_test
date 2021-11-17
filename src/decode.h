#ifndef DECODE_H
#define DECODE_H

#include <string>
#include <functional>
#include "tool.h"



struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
enum   AVMediaType;

//template class __declspec( dllexport ) std::function<int()>; // fix compiler warning. �D�n�O�ץX��dll���ܻݭn�㦡��{�Ƽ˪O



class DLL_API Decode
{
public:

    Decode();
    virtual ~Decode();

    Decode( const Decode& ) = delete;
    Decode( Decode&& ) = delete;

    Decode& operator = ( const Decode& ) = delete;
    Decode& operator = ( Decode&& ) = delete;

    virtual int     init();
    virtual int     end();

    virtual int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx ) = 0;

    int     send_packet( const AVPacket *pkt );
    int     recv_frame();
    void    unref_frame();

    AVFrame*        get_frame();
    AVMediaType     get_decode_context_type();
    AVCodecContext* get_decode_context();

#ifdef FFMPEG_TEST
    int     flush();
    std::function<int()>    output_frame_func;
#endif



protected:

    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx, AVMediaType type );

    AVCodecContext  *dec_ctx    =   nullptr;
    AVFrame         *frame      =   nullptr;

    int     frame_count =   0;

private:

};


#endif