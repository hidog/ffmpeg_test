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
    ~VideoDecode();

    VideoDecode( const VideoDecode& ) = delete;
    VideoDecode( VideoDecode&& ) = delete;

    VideoDecode& operator = ( const VideoDecode& ) = delete;
    VideoDecode& operator = ( VideoDecode&& ) = delete;
    
    int     open_codec_context( AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;
    int     recv_frame( int index ) override;
    
    int     init() override;
    int     end() override;
    void    unref_frame() override;
    
    AVFrame*    get_frame() override;



    int     render_nongraphic_subtitle();
    int     overlap_subtitle_image();

    QImage  get_video_image();

    
    int     get_video_width();
    int     get_video_height();

    void        output_video_frame_info();
    int64_t     get_timestamp();
    int64_t     get_pts( int sec );
    VideoData   output_video_data();
    void        generate_overlay_image();        

    AVPixelFormat   get_pix_fmt();

    int     video_info(); // ���ӼW�[ nv decode �i�H�Ѧҳo��

    void    set_subtitle_decoder( SubDecode *sd );

#ifdef FFMPEG_TEST
    int     output_overlay_by_QT();   // �B�z graphic subtitle
    int     output_jpg_by_QT();
    int     output_jpg_by_openCV();
    void    set_output_jpg_root( std::string _root_path );
#endif

private:

    int64_t     frame_pts   =   0;

    AVMediaType     type;
    AVPixelFormat   pix_fmt;
    SwsContext      *sws_ctx    =   nullptr;   // use for transform yuv to rgb, others.

    uint8_t  *video_dst_data[4]     =   { nullptr };
    int      video_dst_linesize[4]  =   { 0 };
    int      video_dst_bufsize      =   0;

    int      width   =   0;
    int      height  =   0;

#ifdef FFMPEG_TEST
    std::string     output_jpg_root_path    =   "H:\\jpg";
#endif

    SubDecode   *sub_dec    =   nullptr;

    QImage  overlay_image;  // �ΨӳB�z graphic subtitle, overlay �� image.

/*
    ���ӼW�[nv decode �i�H�Ѧҳo��
    AVCodecID   v_codec_id;
    AVPacket *pkt_bsf    =   nullptr;    
    AVBSFContext    *v_bsf_ctx  =   nullptr,;
*/

};




#endif