#ifndef MAKER_DEF_H
#define MAKER_DEF_H


#include <stdint.h>
#include <string>
#include "../IO/io_def.h"


extern "C" {

#include <libavcodec/codec_id.h>

} // end extern "C"




// 目前只會處理一個 v/a/s, 所以用列舉即可
// 如果需要處理多個 a/s, 要改成回傳 stream index 之類.
enum class EncodeOrder : uint8_t 
{
    VIDEO,
    AUDIO,
    SUBTITLE
};



struct EncodeSetting
{
    IO_Type         io_type;
    std::string     filename;
    std::string     extension;
    bool            has_subtitle;
    std::string     srt_ip;     // 保留欄位 目前srt encode是server side.
    std::string     srt_port;
};



struct VideoEncodeSetting
{
    AVCodecID       code_id;
    AVRational      time_base;
    AVPixelFormat   pix_fmt;

    int     width;
    int     height;

    int     gop_size;
    int     max_b_frames;

    int             src_width;
    int             src_height;
    AVPixelFormat   src_pix_fmt;

    std::string     load_jpg_root_path;
};



struct AudioEncodeSetting
{
    AVCodecID   code_id;
    int64_t     bit_rate;
    int         sample_rate;
    uint64_t    channel_layout;
    int         sample_fmt;    

    std::string     load_pcm_path;
};



struct SubEncodeSetting
{
    AVCodecID       code_id;
    std::string     subtitle_file;
    std::string     subtitle_ext;
};



#endif