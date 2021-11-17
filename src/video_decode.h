#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H

#include "decode.h"
#include "tool.h"

#include <QImage>


// https://stackoverflow.com/questions/13088749/efficient-conversion-of-avframe-to-qimage

struct SwsContext;
class QVideoSurfaceFormat;


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

    VideoData   output_video_data();



#ifdef FFMPEG_TEST
    int     output_jpg_by_QT();
    int     output_jpg_by_openCV();
#endif

    int     init() override;
    int     end() override;


    myAVMediaType   get_type() override { return type; } 

    int64_t     get_timestamp();

private:

    const myAVMediaType   type    =   myAVMediaType::AVMEDIA_TYPE_VIDEO;

    uint8_t *video_dst_data[4]  =   {nullptr};
    int     video_dst_linesize[4];
    int     video_dst_bufsize;

    int     width   =   0;
    int     height  =   0;

    myAVPixelFormat pix_fmt    =   myAVPixelFormat::AV_PIX_FMT_NONE;
    SwsContext      *sws_ctx   =   nullptr;   // use for transform yuv to rgb, others.


    //int64 pts_base = 0;


};




#endif