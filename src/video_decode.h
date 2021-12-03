#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H

#include "decode.h"
#include "tool.h"

#include <QImage>


struct  SwsContext;
struct  AVCodec;
struct  AVCodecContext;
enum    AVPixelFormat;


class DLL_API VideoDecode : public Decode
{
public:
    VideoDecode();
    ~VideoDecode();

    VideoDecode( const VideoDecode& ) = delete;
    VideoDecode( VideoDecode&& ) = delete;

    VideoDecode& operator = ( const VideoDecode& ) = delete;
    VideoDecode& operator = ( VideoDecode&& ) = delete;
    
    int     open_codec_context( AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;
    int     recv_frame( int index ) override;

    
    int     init() override;
    int     end() override;
    
    int     get_video_width();
    int     get_video_height();

    void        output_video_frame_info();
    int64_t     get_timestamp();
    VideoData   output_video_data();

    AVPixelFormat   get_pix_fmt();

    int video_info(); // 未來增加 nv decode 可以參考這邊

#ifdef FFMPEG_TEST
    int     output_jpg_by_QT();
    int     output_jpg_by_openCV();
#endif

private:

    AVMediaType     type;
    AVPixelFormat   pix_fmt;
    SwsContext      *sws_ctx    =   nullptr;   // use for transform yuv to rgb, others.

    uint8_t  *video_dst_data[4]     =   { nullptr };
    int      video_dst_linesize[4]  =   { 0 };
    int      video_dst_bufsize      =   0;

    int      width   =   0;
    int      height  =   0;



/*
    未來增加nv decode 可以參考這邊
    AVCodecID   v_codec_id;
    AVPacket *pkt_bsf    =   nullptr;    
    AVBSFContext    *v_bsf_ctx  =   nullptr,;
*/

};




#endif