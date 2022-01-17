#ifndef VIDEO_ENCODE_IO_H
#define VIDEO_ENCODE_IO_H

#include "video_encode.h"


class VideoEncodeIO : public VideoEncode
{
public:
    VideoEncodeIO();
    ~VideoEncodeIO();

    void init( int st_idx, VideoEncodeSetting setting, bool need_global_header ) override;


    void        next_frame() override;



private:
};




#endif