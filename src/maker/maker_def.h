#ifndef MAKER_DEF_H
#define MAKER_DEF_H


#include <stdint.h>
#include <string>



extern "C" {

#include <libavcodec/codec_id.h>

} // end extern "C"




struct EncodeSetting
{
    std::string     filename;
    std::string     extension;
};



struct VideoEncodeSetting
{
    AVCodecID   code_id;
    int         width;
    int         height;
};



struct AudioEncodeSetting
{
    AVCodecID   code_id;
    int64_t     bit_rate;
    int         sample_rate;
};




#endif