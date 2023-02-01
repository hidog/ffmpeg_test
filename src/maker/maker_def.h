#ifndef MAKER_DEF_H
#define MAKER_DEF_H


#include <stdint.h>
#include <string>


extern "C" {

#include <libavcodec/codec_id.h>

} // end extern "C"




constexpr int   default_video_stream_index      =   0;
constexpr int   default_audio_stream_index      =   1;
constexpr int   default_subtitle_stream_index   =   2;



struct EncodeSetting
{
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
    //uint64_t    channel_layout;
    int         channels;
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