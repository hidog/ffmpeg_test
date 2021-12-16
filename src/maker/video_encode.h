#ifndef VIDEO_ENCODE_H
#define VIDEO_ENCODE_H

#include <stdio.h>

struct AVCodec;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;


class VideoEncode
{
public:
    VideoEncode();
    ~VideoEncode();

    VideoEncode( const VideoEncode& ) = delete;
    VideoEncode( VideoEncode&& ) = delete;

    VideoEncode& operator = ( const VideoEncode& ) = delete;
    VideoEncode& operator = ( VideoEncode&& ) = delete;

    void    init();
    void    work();
    void    end();
    void    encode( AVFrame *frame );


private:

    AVCodec *codec = nullptr;
    AVCodecContext *ctx = nullptr;
    AVPacket *pkt = nullptr;
    AVFrame *frame = nullptr;

    FILE *output;
};



#endif