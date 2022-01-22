#ifndef VIDEO_DECODE_HW_H
#define VIDEO_DECODE_HW_H

#include "video_decode.h"



class NvDecoder;
struct AVPacket;
struct AVBSFContext;


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
    int     recv_frame( int index ) override;
    
    VideoData       output_video_data() override;


    AVPixelFormat   get_pix_fmt() override;


    int     init_bsf( AVFormatContext *fmt_ctx );
    int     init_nvidia_decoder();

private:

    NvDecoder   *nv_decoder  =   nullptr;

    bool    use_bsf     =   false;
    
    AVPacket        *pkt_bsf    =   nullptr;    
    AVBSFContext    *v_bsf_ctx  =   nullptr;

    uint8_t **ppFrame = nullptr;
    int nFrameReturned = 0;
    int recv_count = 0;
     
    int64_t *p_timestamp = nullptr;
};



#endif