#ifndef VIDEO_ENCODE_HW_H
#define VIDEO_ENCODE_HW_H


#include "video_encode.h"



struct AVBufferRef;
enum AVHWDeviceType;




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
    int     send_frame() override;
    /*void    end() override;
    int     recv_frame() override;
    void    next_frame() override;
    void    encode_timestamp() override;
    int     flush() override;
    bool    end_of_file() override;
    int64_t get_pts() override;

    AVRational  get_compare_timebase() override;*/

    void    list_hw_decoders();
    int     find_hw_device_type();
    int     hw_encoder_init( const AVHWDeviceType type );
    int     create_hw_encoder( AVCodec* enc );

private:
 
    const   std::string     hw_dev  =   "cuda";
 
    AVPixelFormat   hw_pix_fmt;
    AVHWDeviceType  hw_type; 
    AVBufferRef*    hw_device_ctx   =   nullptr;
    AVFrame*        hw_frame        =   nullptr;

};



#endif