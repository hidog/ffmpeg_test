#ifndef DEMUX_H
#define DEMUX_H

#include <string>
#include "tool.h"

struct AVPacket;
struct AVFormatContext;



/*
https://cloud.tencent.com/developer/article/1333501

直接demux出來的東西不能用ffplay播放
要用 av_bsf_send_packet( v_bsf_ctx, pkt );
     av_bsf_receive_packet( v_bsf_ctx, pkt_bsf );
細節參考nvidia decode官方範例

音效也無法直接播放,但目前尚未找到解法
*/


class DLL_API Demux 
{
public:
    Demux();
    ~Demux();

    Demux( const Demux& ) = delete;
    Demux( Demux&& ) = delete;

    Demux& operator = ( const Demux& ) = delete;
    Demux& operator = ( Demux&& ) = delete;

    int     init();
    int     demux();
    int     end();

    int     open_input( std::string str );
    int     get_stream_info();
    int     get_video_info();
    int     get_audio_info();

    int     get_video_index();
    int     get_audio_index();

    AVPacket*   get_packet();
    void        unref_packet();

    AVFormatContext*    get_format_context();


private:
    AVFormatContext *fmt_ctx    =   nullptr;
    AVPacket        *pkt        =   nullptr;
                    //*pkt_bsf    =   nullptr;    
    //AVBSFContext    *v_bsf_ctx  =   nullptr,;

    int     vs_idx      =   -1;         // video stream index
    int     as_idx      =   -1;         // audio stream index
    //bool    use_bsf     =   false;

    std::string     src_file;
};



#endif