#ifndef DEMUX_H
#define DEMUX_H

#include <string>
#include "tool.h"

struct AVPacket;
struct AVFormatContext;



/*
https://cloud.tencent.com/developer/article/1333501

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