#ifndef SUB_ENCODE_H
#define SUB_ENCODE_H


#include "maker_def.h"
#include "encode.h"


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
    int     open_subtitle_source();

    int64_t     get_pts() override;
    AVFrame*    get_frame() override;

private:

};




#endif