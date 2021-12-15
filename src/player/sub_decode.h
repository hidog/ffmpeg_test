#ifndef SUB_DECODE_H
#define SUB_DECODE_H

#include "decode.h"

/*
https://www.twblogs.net/a/5ef2639e26bc8c4a8eb3c99e        // �r���о� 
http://underpop.online.fr/f/ffmpeg/help/subtitles.htm.gz  // subtitle parameter.
https://programmerclick.com/article/5677107472/
*/


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
    void    flush_for_seek() override;


    int     decode_subtitle( AVPacket* pkt );
    void    generate_subtitle_image( AVSubtitle &subtitle );
    void    output_sub_frame_info();
    void    set_subfile( std::string path );

    bool    open_subtitle_filter( std::string args, std::string desc );
    QImage  get_subtitle_image();
    bool    is_graphic_subtitle();

    int     send_video_frame( AVFrame *video_frame );
    int     render_subtitle();
    int     init_sws_ctx( SubData sd );
    void    switch_subtltle( std::string path );
    void    switch_subtltle( int index );

    void    set_filter_args( std::string args );
    void    set_sub_src_type( SubSourceType type );
    bool    is_video_in_duration( int64_t timestamp );
    QPoint  get_subtitle_image_pos();

    void    init_graphic_subtitle( SubData sd );

    std::string     get_subfile();
    SubSourceType   get_sub_src_type();

    std::pair<std::string,std::string>  get_subtitle_param( AVFormatContext *fmt_ctx, std::string src_file, SubData sd );
    std::vector<std::string>            get_embedded_subtitle_list();


    int sub_info(); // �ثe�L�@��


private:

    AVMediaType     type;    

    AVFilterContext     *bf_src_ctx     =   nullptr;
    AVFilterContext     *bf_sink_ctx    =   nullptr;
    AVFilterGraph       *graph          =   nullptr; 
    SwsContext          *sws_ctx        =   nullptr;  
    std::string         sub_file;
    std::string         subtitle_args;
    SubSourceType       sub_src_type    =   SubSourceType::NONE;

    // general use
    int     video_width, 
            video_height;

    // non-graphic use
    int     sub_index   =   0;
    bool    is_graphic  =   false;

    // use for generate subtitle image.
    double      sub_dpts        =   -1; 
    double      sub_duration    =   -1;
    bool        has_sub_image   =   false;
    int         sub_x, sub_y;

    // v-frame �[�W subtitle, �άO���� subtitle image.
    QImage      sub_image;   

    uint8_t  *sub_dst_data[4]     =   { nullptr };
    int      sub_dst_linesize[4]  =   { 0 };
    int      sub_dst_bufsize      =   0;

};




#endif