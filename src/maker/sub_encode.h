#ifndef SUB_ENCODE_H
#define SUB_ENCODE_H


#include "maker_def.h"
#include "encode.h"


struct AVFormatContext;
struct AVCodecContext;
struct AVStream;


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

    int64_t     get_pts() override;
    AVFrame*    get_frame() override;

private:

    // ¥Î¨ÓÅª¨ú subtitle file.
    AVFormatContext*    fmt_ctx     =   nullptr;
    AVCodecContext*     dec         =   nullptr;
    AVStream*           sub_stream  =   nullptr;

    int     sub_idx =   0;

};




#endif