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
struct AVStream;




/*
    NVENC 出來的資料需要做多次讀取
    用這個結構紀錄存取的位置.
*/
struct NvEncBuffer
{
    std::vector<uint8_t>    enc_data;
    int     read_size;
    int     remain_size;
};


/*
    用 NvEncode 的時候無法透過介面設定 timestamp, duration
    最後用一個 demux 讀取 NvEncode 的 stream, 再解開 time_base 等資訊
    作法不是很好, 會造成初始化的時候需要讀取大量圖片
    目前 study 結果, build ffmpeg with nvenc 應該是比較好的做法.
    ffmpeg 內有 nvenc.c 等相關檔案.

    雖然介面相同, 但內容會跟 VideoEncode 差很多. 有空在思考怎麼處理.
*/

class VideoEncodeHW : public VideoEncode
{
public:
    using EncodedVector =    std::vector<std::vector<uint8_t>>;
    using CUcontext     =   CUctx_st*;

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
    void    encode_timestamp() override;
    int     flush() override;
    bool    end_of_file() override;
    int64_t get_pts() override;

    AVRational  get_compare_timebase() override;

    void    init_nv_encode( uint32_t width, uint32_t height, AVPixelFormat pix_fmt, VideoEncodeSetting setting );
    int     open_convert_demux();  // nvenc 出來的 stream 用 demux 解出 packet, 加上 pts, duration, 再丟入 mux.
    int     get_nv_encode_data( uint8_t *buffer, int size );
    void    init_nv_frame_buffer( VideoEncodeSetting setting );

private:
    static constexpr int    demux_buffer_size   =   65536;

    int     nv_frame_count  =   0;      // 目前只有印 log 功能
    bool    nv_eof          =   false;  // 因為兩個階段, 需要兩個 eof 判斷 stream 是否結束.

    // 為了效能, 計算 pts 的時候用 +=, 不用 *. 紀錄每次增加的 step.
    int64_t     duration_per_frame  =   0;  
    int64_t     duration_count      =   0;   // duration_count = count * duration_per_frame

    // demux
    AVFormatContext     *demux_ctx     =   nullptr;  // 負責將 nvenc 出來的 stream 做 demux.
    AVStream            *nv_stream     =   nullptr;  // 需要做兩次轉換. source frame -> nvenc -> output. nv_stream 用在 mux 前.

    // nvenc.
    NvEncoderCuda   *nv_enc =   nullptr;
    CUcontext       cu_ctx  =   nullptr;  // note: CUcontext = CUctx_st*


    std::list<NvEncBuffer>  nv_encoded_list;    // nvenc 出來的資料存在這個 list, 再讓 demux 讀取.
    EncodedVector           encoded_vec;        // nvenc 編碼後的資料暫存 

    // frame data 傳入 nvenc 的 buffer.
    uint8_t*    nv_data[4]      =   { nullptr };
    int         nv_linesize[4]  =   { 0 };
    int         nv_bufsize      =   0;    
};



// VideoEncodeHW 內的 demux 使用這個 callback function 讀取 NvEncode 出來的 stream.
int     demux_read( void *opaque, uint8_t *buffer, int size );



#endif