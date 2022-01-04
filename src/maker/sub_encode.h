#ifndef SUB_ENCODE_H
#define SUB_ENCODE_H


#include "maker_def.h"
#include "encode.h"
#include <queue>


extern "C" {

#include <libavcodec/packet.h>

} // extern "C"




struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVPacket;



// use struct for compare av packet pts.
struct compare_pkt_by_pts
{
    bool operator () ( AVPacket a, AVPacket b )
    {
        return  a.pts > b.pts;    
    }
};




class SubEncode : public Encode
{
public:
    SubEncode();
    ~SubEncode();

    SubEncode( const SubEncode& ) =   delete;
    SubEncode( SubEncode&& )      =   delete;

    SubEncode& operator = ( const SubEncode& )    =   delete;
    SubEncode& operator = ( SubEncode&& )         =   delete;

    void    init( int st_idx, SubEncodeSetting setting, bool need_global_header );
    void    end();
    int     open_subtitle_source( std::string src_sub_file );
    void    copy_sub_header();
    void    load_all_subtitle();

    int64_t     get_pts() override;
    AVFrame*    get_frame() override;



private:


    // ¥Î¨ÓÅª¨ú subtitle file.
    AVFormatContext*    fmt_ctx     =   nullptr;
    AVCodecContext*     dec         =   nullptr;
    AVStream*           sub_stream  =   nullptr;

    int     sub_idx =   0;

    std::priority_queue< AVPacket, std::vector<AVPacket>, compare_pkt_by_pts >  sub_queue;

};




#endif