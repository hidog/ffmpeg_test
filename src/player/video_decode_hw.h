#ifndef VIDEO_DECODE_HW_H
#define VIDEO_DECODE_HW_H

#include "video_decode.h"



class NvDecoder;
struct AVPacket;
struct AVBSFContext;
enum cudaVideoCodec;
enum AVCodecID;


// ffplay -f rawvideo -pix_fmt nv12 -video_size 1280x720 output.data      yuv420p
// ffplay -f rawvideo -pix_fmt p016 -video_size 1920x1080 2output.data    yuv420p10


// note: 檢查那些資料是 VideoDecode allocate 但忘了釋放的
// note: 暫時不支援 seek.  參考 av_bsf_flush

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
    
    AVPixelFormat   get_pix_fmt() override;

    int     init_bsf( AVFormatContext *fmt_ctx );
    int     init_nvidia_decoder();
    void    convert_to_planar( uint8_t *ptr, int w, int h, int d );


    cudaVideoCodec  codec_id_ffmpeg_to_cuda( AVCodecID id );

private:

    NvDecoder   *nv_decoder  =   nullptr;
    
    AVPacket        *pkt_bsf    =   nullptr;    
    AVBSFContext    *v_bsf_ctx  =   nullptr;
    
    bool        use_bsf         =   false;  
    uint8_t     **nv_frames     =   nullptr;
    int         recv_size       =   0;
    int         recv_count      =   0;     
    int64_t     *p_timestamp    =   nullptr;
};



#endif