#ifndef PLAY_DEF_H
#define PLAY_DEF_H


#include <stdint.h>
#include <QImage>
#include "../IO/io_def.h"


//#define CPU_DECODE
#define HW_DECODE
//#define NV_DECODE




#ifdef FFMPEG_TEST
#define RENDER_SUBTITLE  // 是否要將字幕加進video frame內
#endif




struct DecodeSetting
{
    IO_Type         io_type;
    std::string     filename;
    std::string     subname;           // 外掛字幕檔名
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
    int     channel_layout;
    int     sample_rate;
    int     sample_size;
    int     sample_type;
    int     code_id;
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
    int     code_id;
    int     time_base_num;
    int     time_base_den;
    int     pix_fmt;
};







#endif