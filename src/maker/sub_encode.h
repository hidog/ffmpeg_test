#ifndef SUB_ENCODE_H
#define SUB_ENCODE_H


#include "maker_def.h"
#include "encode.h"
#include <queue>


extern "C" {

#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>

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
    int     get_queue_size();
    int     encode_subtitle();
    void    unref_subtitle();
    void    set_last_pts( int64_t _pts );

    int     flush() override;   
    int     send_frame() override;
    int     recv_frame() override;
    void    encode_timestamp() override;    
    void    unref_data() override;
    void    next_frame() override;

    AVRational  get_compare_timebase() override;
    int64_t     get_pts() override;

    int64_t     get_subtitle_pts();
    int64_t     get_duration();
    AVRational  get_src_stream_timebase();

private:

    // ¥Î¨ÓÅª¨ú subtitle file.
    AVFormatContext*    fmt_ctx     =   nullptr;
    AVCodecContext*     dec         =   nullptr;
    AVStream*           sub_stream  =   nullptr;

    int     sub_idx     =   -1;
    int     got_sub     =   -1;

    std::priority_queue< AVPacket, std::vector<AVPacket>, compare_pkt_by_pts >  sub_queue;

    // use for encode
    AVSubtitle  sub;
    AVPacket    *sub_pkt    =   nullptr;
    int64_t     last_pts    =   0;

    // use for output
    static constexpr int    sub_buf_max_size  =   1024*1024;
    uint8_t     *sub_buf    =   nullptr;
};




#endif