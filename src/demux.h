#ifndef DEMUX_H
#define DEMUX_H

#include <string>
#include <queue>
#include <mutex>

#include "tool.h"

struct AVPacket;
struct AVFormatContext;
enum   AVCodecID;
enum   AVPixelFormat;


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

    //
    int     init();
    int     demux();
    int     end();

#ifdef USE_MT
    std::pair<int,AVPacket*>    demux_multi_thread();
    void    collect_packet( AVPacket *_pkt );
#endif

    //
    int     open_input( std::string str );
    int     stream_info();
    int     video_info();
    int     audio_info();
    int     sub_info();

    bool    exist_subtitle();
    void    set_exist_subtitle( bool flag );

    //
    AVPacket*   get_packet();
    void        unref_packet();

    AVFormatContext*    get_format_context();


    std::pair<std::string,std::string> get_subtitle_param( int v_index, int width, int height, std::string src_file, AVPixelFormat pix_fmt );

private:
    // video
    AVCodecID   v_codec_id;

    // audio
    AVCodecID   a_codec_id;

    // subtitle
    int     current_subtitle_index  =   0;

    //
    AVFormatContext *fmt_ctx    =   nullptr;
    AVPacket        *pkt        =   nullptr;
                    //*pkt_bsf    =   nullptr;    
    //AVBSFContext    *v_bsf_ctx  =   nullptr,;

    // use for multi-thread
#ifdef USE_MT
    AVPacket*               pkt_array[10]  =   {nullptr};  
    std::queue<AVPacket*>   pkt_queue;
    std::mutex              pkt_mtx; 
#endif

    //bool    use_bsf     =   false;

    std::string     src_file;

    bool    exist_subtitle_flag     =   false;
};



#endif