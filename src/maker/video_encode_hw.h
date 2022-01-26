#ifndef VIDEO_ENCODE_HW_H
#define VIDEO_ENCODE_HW_H


#include "video_encode.h"

#include <vector>
#include <list>
#include <stdint.h>

extern "C" {

#include <libavutil/rational.h>

} // end extern "C"


class NvEncoderCuda;
enum AVPixelFormat;
struct CUctx_st;
struct AVFormatContext;
struct AVBSFContext;
struct AVStream;



struct BufferData
{
    std::vector<uint8_t>  data;
    int read_size;
    int remain_size;
};


/*
    �� NvEncode ���ɭԵL�k�z�L�����]�w timestamp, duration
    �̫�Τ@�� demux Ū�� NvEncode �� stream, �A�Ѷ} time_base ����T
    �@�k���O�ܦn, �|�y����l�ƪ��ɭԻݭnŪ���j�q�Ϥ�
    �ثe study ���G, build ffmpeg with nvenc ���ӬO����n�����k.
    ffmpeg ���� nvenc.c �������ɮ�.
*/

class VideoEncodeHW : public VideoEncode
{
public:
    VideoEncodeHW();
    ~VideoEncodeHW();

    VideoEncodeHW( const VideoEncodeHW& ) = delete;
    VideoEncodeHW( VideoEncodeHW&& ) = delete;

    VideoEncodeHW& operator = ( const VideoEncodeHW& ) = delete;
    VideoEncodeHW& operator = ( VideoEncodeHW&& ) = delete;

    void    init( int st_idx, VideoEncodeSetting setting, bool need_global_header ) override;
    void    end() override;
    int     send_frame() override;
    int     recv_frame() override;
    void    next_frame() override;
    int64_t     get_pts() override;

    AVRational  get_compare_timebase() override;

    void    encode_timestamp() override;


    void    init_nv_encode( uint32_t width, uint32_t height, AVPixelFormat pix_fmt );
    void    open_convert_ctx();

    int     get_nv_encode_data( uint8_t *buffer, int size );
    bool    end_of_file() override;

private:

    bool nv_eof = false;


    AVFormatContext     *fmt_ctx     =   nullptr;
    AVStream        *stream = nullptr;

    int decode_count = 0;

    //
    NvEncoderCuda   *nv_enc =   nullptr;
    CUctx_st* cuContext = nullptr; // note: CUcontext = CUctx_st*


    std::vector<std::vector<uint8_t>> vPacket;
    std::list<BufferData>   buffer_list;
    
};



// VideoEncodeHW ���� demux �ϥγo�� callback function Ū�� NvEncode �X�Ӫ� stream.
int     demux_read( void *opaque, uint8_t *buffer, int size );



#endif