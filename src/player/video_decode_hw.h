#ifndef VIDEO_DECODE_HW_H
#define VIDEO_DECODE_HW_H

#include "video_decode.h"


struct AVPacket;
struct AVBufferRef;
struct SwsContext;
enum AVCodecID;
enum AVHWDeviceType;


/*

�ϥ� ffmpeg �� hw decode. 
�i�H�Ѧҩx��d�� hw_decode

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
    int     recv_frame( int index ) override;
    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx, AVMediaType type ) override;


    void    list_hw_decoders();
    int     find_hw_device_type();
    int     create_hw_decoder( AVCodec* dec );
    int     hw_decoder_init( const AVHWDeviceType type );
    
    AVPixelFormat   get_pix_fmt();


#ifdef FFMPEG_TEST
    int     flush() override;
#endif

private:
    const   std::string     hw_dev  =   "cuda";

    AVHWDeviceType  hw_type;
    AVPixelFormat   hw_pix_fmt;

    AVBufferRef*    hw_device_ctx   =   nullptr;
    AVFrame*        hw_frame        =   nullptr;   // cuda frame
    AVFrame*        sw_frame        =   nullptr;   // cuda copy to cpu, ����A�ন���w�� format �� frame.

    SwsContext*     hw_sws_ctx      =   nullptr;   // nv �� yuv420, yuv420p16 ��

};



AVPixelFormat   get_hw_format( AVCodecContext* ctx, const AVPixelFormat* pix_fmts );



#endif