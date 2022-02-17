#ifndef VIDEO_DECODE_HW_H
#define VIDEO_DECODE_HW_H

#include "video_decode.h"

struct AVPacket;
enum AVCodecID;


/*

使用 ffmpeg 的 hw decode. 
可以參考官方範例 hw_decode

*/



class VideoDecodeHW : public VideoDecode 
{
public:
    VideoDecodeHW();
    ~VideoDecodeHW();

    VideoDecodeHW( const VideoDecodeHW& ) = delete;
    VideoDecodeHW( VideoDecodeHW&& ) = delete;

    VideoDecodeHW& operator = ( const VideoDecodeHW& ) = delete;
    VideoDecodeHW& operator = ( VideoDecodeHW&& ) = delete;

    int     init() override;
    int     end() override;
    void    flush_for_seek() override;
    int     open_codec_context( AVFormatContext *fmt_ctx ) override;
    int     send_packet( AVPacket *pkt ) override;
    int     recv_frame( int index ) override;

    void    list_hw_decoders();

#ifdef FFMPEG_TEST
    int     flush() override;
#endif

private:

};



#endif