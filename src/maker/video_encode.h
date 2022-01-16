#ifndef VIDEO_ENCODE_H
#define VIDEO_ENCODE_H


#include "encode.h"
#include "maker_def.h"

#include <stdint.h>



// https://www.itread01.com/content/1550140412.html



struct AVFrame;
struct SwsContext;
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

    void    init( int st_idx, VideoEncodeSetting setting, bool need_global_header );
    void    end();
    void    init_sws( VideoEncodeSetting setting );

    void    list_frame_rate( AVCodecID code_id );
    void    list_pix_fmt( AVCodecID code_id );

    void        next_frame() override;
    int64_t     get_pts() override;

    void    get_fram_from_file_QT();
    void    get_fram_from_file_openCV();

    // for test. run without mux.
    // 目前不能動, 需要修復.
    void    work_test();
    void    encode_test();

private:
    SwsContext      *sws_ctx    =   nullptr;    
    
    uint8_t*    video_data[4]       =   { nullptr };
    int         video_linesize[4]   =   { 0 };
    int         video_bufsize       =   0;

    int     src_width   =   0;
    int     src_height  =   0;

    std::string     load_jpg_root_path  =   "J:\\jpg";
};



#endif