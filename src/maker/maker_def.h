#ifndef MAKER_DEF_H
#define MAKER_DEF_H


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/codec_id.h>

#ifdef __cplusplus
} // end extern "C"
#endif



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