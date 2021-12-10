#ifndef DEMUX_H
#define DEMUX_H

#include <string>
#include <queue>
#include <mutex>

#include "tool.h"

struct AVPacket;
struct AVFormatContext;

/*
����demux�X�Ӫ��F�褣���ffplay����
�n�� av_bsf_send_packet( v_bsf_ctx, pkt );
     av_bsf_receive_packet( v_bsf_ctx, pkt_bsf );
�Ӹ`�Ѧ�nvidia decode�x��d��

���Ĥ]�L�k��������,���ثe�|�����Ѫk
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
    
    int     open_input( std::string src_file );
    int     stream_info();    
    
    AVPacket*   get_packet();
    void        unref_packet();
    int64_t     get_duration_time();

    AVFormatContext*    get_format_context();

#ifdef USE_MT
    std::pair<int,AVPacket*>    demux_multi_thread();
    void    collect_packet( AVPacket *_pkt );
#endif


private:
    
    AVFormatContext *fmt_ctx    =   nullptr;
    AVPacket        *pkt        =   nullptr;

#ifdef USE_MT
    static constexpr int   pkt_size    =   500;
    AVPacket*               pkt_array[pkt_size]  =   {nullptr};  
    std::queue<AVPacket*>   pkt_queue;
    std::mutex              pkt_mtx; 
#endif

};



#endif