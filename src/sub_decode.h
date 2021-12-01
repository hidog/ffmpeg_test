#ifndef SUB_DECODE_H
#define SUB_DECODE_H

#include "decode.h"



struct AVCodec;
struct AVCodecContext;
struct AVFilterContext;
struct AVFilterGraph;
struct AVSubtitle;
struct SwsContext;


class DLL_API SubDecode : public Decode
{
public:
    SubDecode();
    ~SubDecode();

    SubDecode( const SubDecode& ) = delete;
    SubDecode( SubDecode&& ) = delete;

    SubDecode& operator = ( const SubDecode& ) = delete;
    SubDecode& operator = ( SubDecode&& ) = delete;

    int     init() override;
    int     end() override;
    
    int     open_codec_context( AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;
    bool    exist_stream() override;

    int     decode_subtitle( AVPacket* pkt );
    void    generate_subtitle_image( AVSubtitle &subtitle );
    void    init_sub_image( SubData sd );
    void    output_sub_frame_info();
    void    set_subfile( std::string path );

    bool    open_subtitle_filter( std::string args, std::string filterDesc );
    QImage  get_subtitle_image();

    int     send_video_frame( AVFrame *video_frame );
    int     render_subtitle();
    int     init_sws_ctx( SubData sd );

    std::string     get_subfile();
    std::pair<std::string,std::string>  get_subtitle_param( AVFormatContext *fmt_ctx, std::string src_file, SubData sd );


    int sub_info(); // 目前無作用


private:

    AVMediaType     type;    

    AVFilterContext     *bf_src_ctx     =   nullptr;
    AVFilterContext     *bf_sink_ctx    =   nullptr;
    AVFilterGraph       *graph          =   nullptr; 
    SwsContext          *sws_ctx        =   nullptr;  
    std::string         sub_file;

    QImage  sub_image;     // 將video frame打上字幕後存在這邊

    uint8_t  *sub_dst_data[4]     =   { nullptr };
    int      sub_dst_linesize[4]  =   { 0 };
    int      sub_dst_bufsize      =   0;

};




#endif