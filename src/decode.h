#ifndef DECODE_H
#define DECODE_H

#include <string>
#include <functional>
#include "tool.h"



struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;


template class __declspec( dllexport ) std::function<int()>; // fix compiler warning. 主要是匯出成dll的話需要顯式具現化樣板



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
    virtual int     output_frame() = 0;
    virtual void    print_finish_message() = 0;


    int     send_packet( const AVPacket *pkt );
    int     recv_frame();
    void    unref_frame();
    int     flush();

    AVFrame*        get_frame();
    myAVMediaType   get_decode_context_type();

    int     open_output( std::string dst );
    
    std::function<int()>    output_frame_func;

    virtual myAVMediaType   get_type() = 0; 

protected:

    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx, myAVMediaType type );

    AVCodecContext  *dec_ctx    =   nullptr;
    AVFrame         *frame      =   nullptr;

    int     frame_count =   0;
    FILE    *dst_fp   =   nullptr;

    std::string     dst_file;

private:

};


#endif