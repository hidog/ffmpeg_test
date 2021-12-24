#ifndef VIDEO_ENCODE_H
#define VIDEO_ENCODE_H


#include "encode.h"
#include "maker_def.h"

#include <stdint.h>



// https://www.itread01.com/content/1550140412.html



struct AVFrame;
enum AVCodecID;



class VideoEncode : public Encode
{
public:
    VideoEncode();
    ~VideoEncode();

    VideoEncode( const VideoEncode& ) = delete;
    VideoEncode( VideoEncode&& ) = delete;

    VideoEncode& operator = ( const VideoEncode& ) = delete;
    VideoEncode& operator = ( VideoEncode&& ) = delete;

    void    init( int st_idx, VideoEncodeSetting setting );
    void    end();

    int64_t     get_pts() override;
    AVFrame*    get_frame() override;

    int         send_frame() override;

    // for test. run without mux.
    // 目前不能動, 需要修復.
    void    work_test();
    void    encode_test();

private:

    uint8_t  *video_dst_data[4]     =   { nullptr };
    int      video_dst_linesize[4]  =   { 0 };
    int      video_dst_bufsize      =   0;

};



#endif