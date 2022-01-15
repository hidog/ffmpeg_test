#ifndef PLAY_DEF_H
#define PLAY_DEF_H


#include <stdint.h>
#include <QImage>
#include "../IO/io_def.h"





struct MediaInfo
{
    // video
    int     width;
    int     height;
    int     time_num;
    int     time_den;
    int     pix_fmt;

    // audio
    int     channel_layout;
    int     sample_rate;
    int     sample_fmt;
};




struct DecodeSetting
{
    IO_Type         io_type;
    
    std::string     filename;
    std::string     subname;           // �~���r���ɦW

    std::string     srt_ip;
    std::string     srt_port;
};





enum class SubSourceType
{
    NONE,
    FROM_FILE,
    EMBEDDED,
};



struct SubData
{
    int     video_index;
    int     width;
    int     height;
    int     sub_index;
    int     pix_fmt;
};



struct AudioData
{
    uint8_t     *pcm;
    int         bytes;
    int64_t     timestamp;
};


struct AudioDecodeSetting
{
    int     channel;
    int     sample_rate;
    //int     sample_size;
};



struct VideoData
{
    int         index;
    QImage      frame;
    int64_t     timestamp;
};



struct VideoDecodeSetting
{
    int     width;
    int     height;
};




#endif