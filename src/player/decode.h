#ifndef DECODE_H
#define DECODE_H

#include <functional>
#include "tool.h"


struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVCodec;
struct AVStream;
enum   AVMediaType;


/*
ffmpeg �d�� demuxing_decoding ������ raw data �����O.
*/


class DLL_API Decode
{
public:

    Decode( AVMediaType _type );
    virtual ~Decode();

    Decode( const Decode& ) = delete;
    Decode( Decode&& ) = delete;

    Decode& operator = ( const Decode& ) = delete;
    Decode& operator = ( Decode&& ) = delete;
    
    virtual void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) = 0;
    //virtual int     open_codec_context( AVFormatContext *fmt_ctx ) = 0;

    virtual int     init();
    virtual int     end();  
    virtual bool    exist_stream();
    virtual void    flush_for_seek();
    virtual void    flush_all_stream();
    
    virtual AVFrame*    get_frame();

    virtual int     send_packet( AVPacket *pkt );
    virtual int     recv_frame( int index );
    virtual void    unref_frame();

    int     get_frame_count();
    bool    find_index( int index );
    int     current_index();
    int     get_dec_map_size();
    void    set_is_current( bool flag );

    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx, AVMediaType type );

    AVMediaType     get_decode_context_type();
    AVCodecContext* get_decode_context();
    AVStream*       get_stream();

#ifdef FFMPEG_TEST
    virtual int     flush();
    std::function<int()>    output_frame_func;
#endif

protected:
    AVMediaType     type;

    int     open_all_codec( AVFormatContext *fmt_ctx, AVMediaType type );

    // ���ŧ令 private + protected interface.
        
    /*
        ���Ӧp�G�n�䴩 multi stream, �̦n�g�@�� decoder manager.
        �ثe�Ȯɶ�b decode ���U.
        �ثe�٤���ܦn���������y. �]�٤��༽��­�����. ���Ӽg decoder manager ���ɭԤ@�_�B�z.

        ���F��K���g, dec_ctx, stream �o��ӬO�ΨӰO�� current decoder and stream. 
        �W�[ decoder manager ���ɭԳo���n���s�]�p
    */
    //std::map<int,AVCodecContext*>   dec_map;
    //std::map<int,AVStream*>         stream_map;
    
    //int     cs_index    =   -1;     // current stream index.

    AVCodecContext  *dec_ctx    =   nullptr;    
    AVStream        *stream     =   nullptr;
    AVFrame         *frame      =   nullptr;
   
    int     frame_count =   -1;
    bool    is_current  =   false;

};


#endif