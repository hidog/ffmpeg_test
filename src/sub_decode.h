#ifndef SUB_DECODE_H
#define SUB_DECODE_H

#include "decode.h"


struct SwrContext;
struct AVCodec;
struct AVCodecContext;
struct AVFilterContext;
struct AVFilterGraph;
struct AVSubtitle;
struct SwsContext;


//enum AVSampleFormat;
enum AVPixelFormat;






// 試著要轉sample rate但沒成功,有機會再試試看.

class DLL_API SubDecode : public Decode
{
public:
    SubDecode();
    ~SubDecode();

    SubDecode( const SubDecode& ) = delete;
    SubDecode( SubDecode&& ) = delete;

    SubDecode& operator = ( const SubDecode& ) = delete;
    SubDecode& operator = ( SubDecode&& ) = delete;

    //
    int     open_codec_context( AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;

    int     decode_subtitle( AVPacket* pkt );
    void    generate_subtitle_image( AVSubtitle &subtitle );


    std::pair<std::string,std::string>  get_subtitle_param( AVFormatContext *fmt_ctx, int v_index, int width, int height, std::string src_file, AVPixelFormat pix_fmt, int current_subtitle_index );


    //
    int     init() override;
    int     end() override;

    void    init_sub_image( int width, int height );

    //
    void    output_sub_frame_info();

    //
    SubData   output_sub_data();

    bool    open_subtitle_filter( std::string args, std::string filterDesc );


    int     generate_subtitle_image( AVFrame *video_frame, SwsContext *sws_ctx );
    QImage  get_subtitle_image();


    int send_video_frame( AVFrame *video_frame );
    int render_subtitle();
    int init_sws_ctx( int width, int height, AVPixelFormat pix_fmt );
    int flush( AVFrame *video_frame );


private:

    AVMediaType     type;    

    AVFilterContext *buffersrcContext = nullptr;
    AVFilterContext *buffersinkContext = nullptr;
    AVFilterGraph *filterGraph = nullptr; //avfilter_graph_alloc();


    QImage  sub_image;     // 如果字幕資料是圖片, 將字幕資料轉成圖片後暫存在這邊

    SwsContext      *sws_ctx   =   nullptr;  


};




#endif