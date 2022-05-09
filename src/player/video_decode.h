#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H

#include "decode.h"
#include "tool.h"
#include <QImage>

class SubDecode;
struct  SwsContext;
struct  AVCodec;
struct  AVCodecContext;
enum    AVPixelFormat;


class DLL_API VideoDecode : public Decode
{
public:
    VideoDecode();
    virtual ~VideoDecode();

    VideoDecode( const VideoDecode& ) = delete;
    VideoDecode( VideoDecode&& ) = delete;

    VideoDecode& operator = ( const VideoDecode& ) = delete;
    VideoDecode& operator = ( VideoDecode&& ) = delete;
    
    virtual AVPixelFormat   get_pix_fmt();

    virtual int     init() override;
    virtual int     end() override;

    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;
    int     recv_frame( int index ) override;
    void    unref_frame() override;
    
    AVFrame*    get_frame() override;

    bool    is_attached();
    int     render_nongraphic_subtitle();
    int     overlap_subtitle_image();
    
    int     get_video_width();
    int     get_video_height();
    int     get_video_codec();
    int     get_video_time_base_num();
    int     get_video_time_base_den();

    void    output_video_frame_info();
    void    generate_overlay_image();        
    void    set_subtitle_decoder( SubDecode *sd );

    int64_t     get_timestamp();
    int64_t     get_pts( int sec );
    VideoData   output_video_data();
    QImage      get_video_image();

    int     video_info(); // 未來增加 nv decode 可以參考這邊 

#ifdef FFMPEG_TEST
    int     flush() override;
    int     output_overlay_by_QT();   // 處理 graphic subtitle
    int     output_jpg_by_QT();
    int     output_jpg_by_openCV();
    int     test_image_process();
    void    set_output_jpg_path( std::string _path );
#endif

//private:
protected:

    int64_t     frame_pts   =   0;

    AVPixelFormat   pix_fmt;
    SwsContext      *sws_ctx    =   nullptr;   // use for transform yuv to rgb, others.

    uint8_t  *video_dst_data[4]     =   { nullptr };
    int      video_dst_linesize[4]  =   { 0 };
    int      video_dst_bufsize      =   0;

    int      width   =   0;
    int      height  =   0;

    SubDecode   *sub_dec    =   nullptr;
    QImage      overlay_image;  // 用來處理 graphic subtitle, overlay 的 image.

#ifdef FFMPEG_TEST
    std::string     output_jpg_path    =   "H:\\jpg";
#endif
};


VideoDecode*    get_video_decoder_instance();



#endif