#ifndef VIDEO_ENCODE_H
#define VIDEO_ENCODE_H

#include <stdio.h>
#include <stdint.h>

struct AVCodec;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;

struct SwsContext;


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
    void    encode( AVFrame *fr );

    int64_t get_next_pts();
    AVFrame* get_frame();

    int send_frame( AVFrame* fr );
    int recv_frame();
    AVPacket* get_pkt();


//private:

    AVCodec *codec = nullptr;
    AVCodecContext *ctx = nullptr;
    AVPacket *pkt = nullptr;
    AVFrame *frame = nullptr;

    FILE *output;


    uint8_t  *video_dst_data[4]     =   { nullptr };
    int      video_dst_linesize[4]  =   { 0 };
    int      video_dst_bufsize      =   0;

    SwsContext* sws_ctx = nullptr;


};



#endif