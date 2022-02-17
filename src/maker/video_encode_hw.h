#ifndef VIDEO_ENCODE_HW_H
#define VIDEO_ENCODE_HW_H


#include "video_encode.h"




class VideoEncodeHW : public VideoEncode
{
public:
    VideoEncodeHW();
    ~VideoEncodeHW();

    VideoEncodeHW( const VideoEncodeHW& ) = delete;
    VideoEncodeHW( VideoEncodeHW&& ) = delete;

    VideoEncodeHW& operator = ( const VideoEncodeHW& ) = delete;
    VideoEncodeHW& operator = ( VideoEncodeHW&& ) = delete;

    /*void    init( int st_idx, VideoEncodeSetting setting, bool need_global_header ) override;
    void    end() override;
    int     send_frame() override;
    int     recv_frame() override;
    void    next_frame() override;
    void    encode_timestamp() override;
    int     flush() override;
    bool    end_of_file() override;
    int64_t get_pts() override;

    AVRational  get_compare_timebase() override;*/

private:
 
};



#endif