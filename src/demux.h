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



    int     get_video_width();
    int     get_video_height();
    int     get_audio_channel();
    int     get_audio_sample_rate();

    //
    int     get_video_index();
    int     get_audio_index();
    int     get_sub_index();

    bool    exist_subtitle();
    void    set_exist_subtitle( bool flag );

    //
    AVPacket*   get_packet();
    void        unref_packet();

    AVFormatContext*    get_format_context();


    std::pair<std::string,std::string> get_subtitle_param( std::string src_file, AVPixelFormat pix_fmt );

private:
    // video
    int     width       =   0;
    int     height      =   0;
    int     depth       =   0;

    AVCodecID   v_codec_id;

    // audio
    int     channel         =   0;
    int     sample_rate     =   0;
    //int     sample_size     =   0;

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

    //
    int     vs_idx      =   -1;         // video stream index
    int     as_idx      =   -1;         // audio stream index
    int     ss_idx      =   -1;         // sub stream index
    //bool    use_bsf     =   false;

    std::string     src_file;

    bool    exist_subtitle_flag     =   false;
};



#endif