#ifndef SUBTITLE_ENCODE_H
#define SUBTITLE_ENCODE_H


#include "maker_def.h"
#include "encode.h"


class SubtitleEncode : public Encode
{
public:
    SubtitleEncode();
    ~SubtitleEncode();

    SubtitleEncode( const SubtitleEncode& ) =   delete;
    SubtitleEncode( SubtitleEncode&& )      =   delete;

    SubtitleEncode& operator = ( const SubtitleEncode& )    =   delete;
    SubtitleEncode& operator = ( SubtitleEncode&& )         =   delete;

    void    init( int st_idx, SubtitleEncodeSetting setting, bool need_global_header );

    int64_t     get_pts() override;
    AVFrame*    get_frame() override;

private:

};




#endif