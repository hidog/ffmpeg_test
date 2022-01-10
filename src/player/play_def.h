#ifndef PLAY_DEF_H
#define PLAY_DEF_H


#include <stdint.h>
#include <QImage>




enum class IO_Type
{
    DEFAULT,
    FILE_IO,
    SRT_IO,
};





struct DecodeSetting
{
    IO_Type         io_type;
    std::string     filename;
    std::string     subname;           // •~±æ¶rπı¿…¶W
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