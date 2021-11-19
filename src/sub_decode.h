#ifndef SUB_DECODE_H
#define SUB_DECODE_H

#include "decode.h"


struct SwrContext;
struct AVCodec;
struct AVCodecContext;
enum AVSampleFormat;


// 試著要轉sample rate但沒成功,有機會再試試看.

class DLL_API SubDecode : public Decode
{
public:
    SubDecode();
    ~SubDecode();

    SubDecode( const SubDecode& ) = delete;
    SubDecode( SubDecode&& ) = delete;

    SubDecode& operator = ( const SubDecode& ) = delete;
    SubDecode& operator = ( SubDecode&& ) = delete;

    //
    int     open_codec_context( int stream_index, AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;

    //
    int     init() override;
    int     end() override;

    //
    void    output_sub_frame_info();

    //
    SubData   output_sub_data();

private:

    AVMediaType     type;    
    //SwrContext      *swr_ctx    =   nullptr; // use for chagne audio data to play.

};




#endif