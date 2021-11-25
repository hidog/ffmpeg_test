#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H

#include "decode.h"
#include "tool.h"

#include <QImage>


// https://stackoverflow.com/questions/13088749/efficient-conversion-of-avframe-to-qimage

struct SwsContext;
struct AVCodec;
struct AVCodecContext;
enum AVPixelFormat;


class DLL_API VideoDecode : public Decode
{
public:
    VideoDecode();
    ~VideoDecode();

    VideoDecode( const VideoDecode& ) = delete;
    VideoDecode( VideoDecode&& ) = delete;

    VideoDecode& operator = ( const VideoDecode& ) = delete;
    VideoDecode& operator = ( VideoDecode&& ) = delete;

    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;

    int     init() override;
    int     end() override;

    void        output_video_frame_info();
    int64_t     get_timestamp();

    VideoData   output_video_data();

    SwsContext* get_sws_ctx() { return sws_ctx; }

    AVPixelFormat   get_pix_fmt();



    int num, den;


#ifdef FFMPEG_TEST
    int     output_jpg_by_QT();
    int     output_jpg_by_openCV();
#endif



private:

    AVMediaType   type;   

    uint8_t *video_dst_data[4]  =   {nullptr};
    int     video_dst_linesize[4];
    int     video_dst_bufsize;

    int     width   =   0;
    int     height  =   0;

    AVPixelFormat   pix_fmt;
    SwsContext      *sws_ctx   =   nullptr;   // use for transform yuv to rgb, others.

};




#endif