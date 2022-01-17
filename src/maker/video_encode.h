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

    void    list_frame_rate( AVCodecID code_id );
    void    list_pix_fmt( AVCodecID code_id );
    
    void        next_frame() override;
    int64_t     get_pts() override;
    void        unref_data() override;


#ifdef FFMPEG_TEST
    void    init_sws( VideoEncodeSetting setting );
    void    get_fram_from_file_QT();
    void    get_fram_from_file_openCV();

    // 這兩個測試用, 目前不能動
    void    encode_test();
    void    work_test();
#endif


protected:

    int     src_width   =   0;
    int     src_height  =   0;

#ifdef FFMPEG_TEST
    uint8_t*    video_data[4]       =   { nullptr };
    int         video_linesize[4]   =   { 0 };
    int         video_bufsize       =   0;

    SwsContext      *sws_ctx    =   nullptr;
    std::string     load_jpg_root_path  =   "J:\\jpg";
#endif
};



#endif