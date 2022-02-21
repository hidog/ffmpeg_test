#ifndef VIDEO_ENCODE_HW_H
#define VIDEO_ENCODE_HW_H


#include "video_encode.h"



struct AVBufferRef;
enum AVHWDeviceType;



/*

https://www.cnblogs.com/betterwgo/p/6843390.html    不完整 但可以參考

*/


class VideoEncodeHW : public VideoEncode
{
public:
    VideoEncodeHW();
    ~VideoEncodeHW();

    VideoEncodeHW( const VideoEncodeHW& ) = delete;
    VideoEncodeHW( VideoEncodeHW&& ) = delete;

    VideoEncodeHW& operator = ( const VideoEncodeHW& ) = delete;
    VideoEncodeHW& operator = ( VideoEncodeHW&& ) = delete;

    void    init( int st_idx, VideoEncodeSetting setting, bool need_global_header ) override;
    void    end() override;

    void    list_hw_decoders();
    int     find_hw_device_type();
    int     hw_encoder_init( const AVHWDeviceType type );

private:
 
    const   std::string     hw_dev  =   "cuda";
 
    AVPixelFormat   hw_pix_fmt;
    AVHWDeviceType  hw_type; 
    AVBufferRef*    hw_device_ctx   =   nullptr;

};



#endif