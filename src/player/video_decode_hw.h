#ifndef VIDEO_DECODE_HW_H
#define VIDEO_DECODE_HW_H

#include "video_decode.h"





// ffplay -f rawvideo -pix_fmt nv12 -video_size 1280x720 output.data
// ffplay -f rawvideo -pix_fmt p016 -video_size 1920x1080 2output.data



class VideoDecodeHW : public VideoDecode 
{
public:
    VideoDecodeHW();
    ~VideoDecodeHW();

    VideoDecodeHW( const VideoDecodeHW& ) = delete;
    VideoDecodeHW( VideoDecodeHW&& ) = delete;

    VideoDecodeHW& operator = ( const VideoDecodeHW& ) = delete;
    VideoDecodeHW& operator = ( VideoDecodeHW&& ) = delete;

    int     open_codec_context( AVFormatContext *fmt_ctx ) override;
    int     init() override;
    int     end() override;
    int     send_packet( AVPacket *pkt ) override;


    int     init_bsf( AVFormatContext *fmt_ctx );


private:

    bool    use_bsf     =   false;
    
    AVPacket        *pkt_bsf    =   nullptr;    
    AVBSFContext    *v_bsf_ctx  =   nullptr;
};



#endif