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
    NVENC �X�Ӫ���ƻݭn���h��Ū��
    �γo�ӵ��c�����s������m.
*/
struct NvEncBuffer
{
    std::vector<uint8_t>    enc_data;
    int     read_size;
    int     remain_size;
};


/*
    �� NvEncode ���ɭԵL�k�z�L�����]�w timestamp, duration
    �̫�Τ@�� demux Ū�� NvEncode �� stream, �A�Ѷ} time_base ����T
    �@�k���O�ܦn, �|�y����l�ƪ��ɭԻݭnŪ���j�q�Ϥ�
    �ثe study ���G, build ffmpeg with nvenc ���ӬO����n�����k.
    ffmpeg ���� nvenc.c �������ɮ�.

    ���M�����ۦP, �����e�|�� VideoEncode �t�ܦh. ���Ŧb��ҫ��B�z.
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
    int     open_convert_demux();  // nvenc �X�Ӫ� stream �� demux �ѥX packet, �[�W pts, duration, �A��J mux.
    int     get_nv_encode_data( uint8_t *buffer, int size );
    void    init_nv_frame_buffer( VideoEncodeSetting setting );

private:
    static constexpr int    demux_buffer_size   =   65536;

    int     nv_frame_count  =   0;      // �ثe�u���L log �\��
    bool    nv_eof          =   false;  // �]����Ӷ��q, �ݭn��� eof �P�_ stream �O�_����.

    // ���F�į�, �p�� pts ���ɭԥ� +=, ���� *. �����C���W�[�� step.
    int64_t     duration_per_frame  =   0;  
    int64_t     duration_count      =   0;   // duration_count = count * duration_per_frame

    // demux
    AVFormatContext     *demux_ctx     =   nullptr;  // �t�d�N nvenc �X�Ӫ� stream �� demux.
    AVStream            *nv_stream     =   nullptr;  // �ݭn���⦸�ഫ. source frame -> nvenc -> output. nv_stream �Φb mux �e.

    // nvenc.
    NvEncoderCuda   *nv_enc =   nullptr;
    CUcontext       cu_ctx  =   nullptr;  // note: CUcontext = CUctx_st*


    std::list<NvEncBuffer>  nv_encoded_list;    // nvenc �X�Ӫ���Ʀs�b�o�� list, �A�� demux Ū��.
    EncodedVector           encoded_vec;        // nvenc �s�X�᪺��ƼȦs 

    // frame data �ǤJ nvenc �� buffer.
    uint8_t*    nv_data[4]      =   { nullptr };
    int         nv_linesize[4]  =   { 0 };
    int         nv_bufsize      =   0;    
};



// VideoEncodeHW ���� demux �ϥγo�� callback function Ū�� NvEncode �X�Ӫ� stream.
int     demux_read( void *opaque, uint8_t *buffer, int size );



#endif