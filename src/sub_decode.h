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

enum AVSampleFormat;






// �յۭn��sample rate���S���\,�����|�A�ոլ�.

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
    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;

    int     decode_subtitle( AVPacket* pkt );
    void    generate_subtitle_image( AVSubtitle &subtitle );

    //
    int     init() override;
    int     end() override;

    //
    void    output_sub_frame_info();

    //
    SubData   output_sub_data();

    bool    open_subtitle_filter( std::string args, std::string filterDesc );


    int     generate_subtitle_image( AVFrame *video_frame, SwsContext *sws_ctx );
    QImage  get_subtitle_image();


private:

    AVMediaType     type;    
    //SwrContext      *swr_ctx    =   nullptr;

    AVFilterContext *buffersrcContext = nullptr;
    AVFilterContext *buffersinkContext = nullptr;

    QImage  sub_image;     // �p�G�r����ƬO�Ϥ�, �N�r������ন�Ϥ���Ȧs�b�o��

};




#endif