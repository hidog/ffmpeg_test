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
    用 NvEncode 的時候無法透過介面設定 timestamp, duration
    最後用一個 demux 讀取 NvEncode 的 stream, 再解開 time_base 等資訊
    作法不是很好, 會造成初始化的時候需要讀取大量圖片
    目前 study 結果, build ffmpeg with nvenc 應該是比較好的做法.
    ffmpeg 內有 nvenc.c 等相關檔案.
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



// VideoEncodeHW 內的 demux 使用這個 callback function 讀取 NvEncode 出來的 stream.
int     demux_read( void *opaque, uint8_t *buffer, int size );



#endif