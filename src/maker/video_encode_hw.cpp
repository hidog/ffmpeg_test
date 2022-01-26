#include "video_encode_hw.h"
#include "tool.h"

#include "hw/NvEncoderCuda.h"
#include "hw/NvEncoderCLIOptions.h"



extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

} // extern "C"







/*******************************************************************************
VideoEncodeHW::VideoEncodeHW()
********************************************************************************/
VideoEncodeHW::VideoEncodeHW()
    :   VideoEncode()
{}




/*******************************************************************************
VideoEncodeHW::~VideoEncodeHW()
********************************************************************************/
VideoEncodeHW::~VideoEncodeHW()
{
    end();
}




/*******************************************************************************
VideoEncodeHW::~VideoEncodeHW()
********************************************************************************/
void    VideoEncodeHW::end()
{
    nv_frame_count  =   0;
    nv_eof          =   false;

    duration_per_frame  =   0;
    duration_count      =   0;

    if( demux_ctx != nullptr )
    {
        avformat_close_input( &demux_ctx );
        demux_ctx   =   nullptr;
    }

    nv_stream   =   nullptr;

    //
    if( nv_enc != nullptr )
    {
        nv_enc->DestroyEncoder();
        delete nv_enc;
        nv_enc  =   nullptr;
    }

    cu_ctx  =   nullptr;

    assert( nv_encoded_list.empty() == true );

    if( nv_data[0] != nullptr )
    {
        av_free( nv_data[0] );
        nv_data[0]   =   nullptr;
        nv_data[1]   =   nullptr;
        nv_data[2]   =   nullptr;
        nv_data[3]   =   nullptr;
    }

    nv_linesize[0]   =   0;
    nv_linesize[1]   =   0;
    nv_linesize[2]   =   0;
    nv_linesize[3]   =   0;
    nv_bufsize       =   0;

    if( nv_sws != nullptr )
    {
        sws_freeContext( nv_sws );
        nv_sws  =   nullptr;
    }

    VideoEncode::end();
}





/*******************************************************************************
io_read_data
********************************************************************************/
int     demux_read( void *opaque, uint8_t *buffer, int size )
{
    VideoEncodeHW   *video_enc_hw   =   (VideoEncodeHW*)opaque;
    assert( video_enc_hw != nullptr );

    int     read_size   =   0;
    
    while( true )
    {
        read_size   =   video_enc_hw->get_nv_encode_data( buffer, size );
        if( read_size > 0 )
            break;
        else if( read_size == EOF )        
            break;        
    }

    if( read_size == EOF )
        MYLOG( LOG::L_INFO, "load nvenc stream finish." )

    return  read_size;
}




/*******************************************************************************
VideoEncodeHW::get_encode_data
********************************************************************************/
int     VideoEncodeHW::get_nv_encode_data( uint8_t *buffer, int size )
{
    // 使用 VideoEncode 介面讀取 frame, 再用 nvenc encode.
    VideoEncode::next_frame();

    // 
    const NvEncInputFrame   *nv_input_frame     =   nv_enc->GetNextInputFrame();
    if( nv_input_frame == nullptr )
        MYLOG( LOG::L_ERROR, "nv_input_frame is nullptr." )

    /*
        NOTE: NvEncodeHW 因為某些理由, 需要兩個 eof
        一個是讀取 frame 的 eof.
        一個是 stream 結束的 eof.
    */
    if( eof_flag == true )    
        nv_enc->EndEncode( encoded_vec );  // flush.
    else
    {
        int     width   =   nv_enc->GetEncodeWidth();
        int     height  =   nv_enc->GetEncodeHeight();

        // convert frame to pointer.
        AVPixelFormat   nv_fmt =   static_cast<AVPixelFormat>( nv_enc->get_ffmpeg_pixel_format() );  
        av_image_fill_pointers( frame->data, nv_fmt, height, nv_data[0], nv_linesize );

        NvEncoderCuda::CopyToDeviceFrame( cu_ctx, nv_data[0], 0, (CUdeviceptr)nv_input_frame->inputPtr,
                                          (int)nv_input_frame->pitch, width, height, CU_MEMORYTYPE_HOST, 
                                          nv_input_frame->bufferFormat, nv_input_frame->chromaOffsets, nv_input_frame->numChromaPlanes );

        nv_enc->EncodeFrame(encoded_vec);
    }

    // 晚點再來重構這一塊.
    if( encoded_vec.size() > 0 )    
    {
        for( int i = 0; i < encoded_vec.size(); i++ )
        {
            NvEncBuffer item;
            item.enc_data = encoded_vec[i];
            item.read_size = 0;
            item.remain_size = encoded_vec[i].size();
            
            nv_encoded_list.emplace_back( item );
        }
    }

    if( nv_encoded_list.size() == 0 && eof_flag == true )
        return EOF;
    if( nv_encoded_list.size() == 0 )
        return  0;
    else
    {
        while( true )
        {
            NvEncBuffer &item = nv_encoded_list.front();
            if( item.remain_size <= size )
            {
                memcpy( buffer, item.enc_data.data() + item.read_size, item.remain_size );
                int read_size = item.remain_size;
                nv_encoded_list.pop_front();
                return read_size;
            }
            else
            {
                memcpy( buffer, item.enc_data.data() + item.read_size, size );
                item.read_size += size;
                item.remain_size -= size;
                return size;
            }
        }
    }


    return 0;
}






/*******************************************************************************
VideoEncodeHW::open_convert_demux()

nvenc 出來的 stream 用 demux 解出 packet, 加上 pts, duration, 再丟入 mux.
********************************************************************************/
int    VideoEncodeHW::open_convert_demux()
{   
    int  ret    =   0;

    assert( demux_ctx == nullptr );
    demux_ctx   =   avformat_alloc_context();    
    if( demux_ctx == nullptr )
    {
        MYLOG( LOG::L_ERROR, "demux_ctx alloc fail." );
        return  R_ERROR;
    }

	//
    uint8_t     *demux_buffer   =   (uint8_t*)av_malloc(demux_buffer_size);
    assert( demux_buffer != nullptr );

	AVIOContext     *io_ctx     =   avio_alloc_context( demux_buffer, demux_buffer_size, 0, (void*)this, demux_read, nullptr, nullptr );
    assert( io_ctx != nullptr );

	AVInputFormat   *input_fmt  =   nullptr;
    ret     =   av_probe_input_buffer( io_ctx, &input_fmt, nullptr, nullptr, 0, 0 );
    assert( ret == 0 );
	demux_ctx->pb =   io_ctx;
    ret         =   avformat_open_input( &demux_ctx, nullptr, input_fmt, nullptr );
    assert( ret == 0 );

    demux_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;  

    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "Could not open" );

    //
    ret     =   avformat_find_stream_info( demux_ctx, nullptr );
    if( ret < 0) 
        MYLOG( LOG::L_ERROR, "Could not find stream information. ret = %d", ret );

    // nv_stream 會用在 mux 階段.
    nv_stream   =   demux_ctx->streams[0];  // 理論上只會有一個 stream, 而且是 video stream.
    assert( nv_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO );

    // 需要將資訊 copy 進去 ctx. mux 的時候會再從 ctx 讀取.
    // 這邊的 ctx 只用來存資訊, 不會拿來 encode.
    ret     =   avcodec_parameters_to_context( ctx, nv_stream->codecpar );
    assert( ret >= 0 );
}




#ifdef FFMPEG_TEST
/*******************************************************************************
VideoEncodeHW::init_sws()

跟 VideoEncode 只有一點差別, 大致上相同.
********************************************************************************/
void    VideoEncodeHW::init_sws( VideoEncodeSetting setting )
{
    if( nv_enc == nullptr )
        MYLOG( LOG::L_ERROR, "nv_enc not init." )

    nv_bufsize  =   av_image_alloc( nv_data, nv_linesize, setting.width, setting.height, setting.pix_fmt, 1 );
    
    int     width   =   nv_enc->GetEncodeWidth();
    int     height  =   nv_enc->GetEncodeHeight();

    // 無法直接存取 pix_fmt.
    // NV_ENC_BUFFER_FORMAT    enc_format  =    nv_enc->GetPixelFormat();

    nv_sws      =   sws_getContext( setting.src_width, setting.src_height, setting.src_pix_fmt,    // src
                                    width,             height,             setting.pix_fmt,        // dst
                                    SWS_BICUBIC, NULL, NULL, NULL );
}
#endif





/*******************************************************************************
VideoEncodeHW::init()
********************************************************************************/
void    VideoEncodeHW::init( int st_idx, VideoEncodeSetting setting, bool need_global_header )
{
    int     ret     =   0;
    
    Encode::init( st_idx, setting.code_id );
    init_nv_encode( setting.width, setting.height, setting.pix_fmt, setting );  // 執行順序不能隨便改, 有相依性.

#ifdef FFMPEG_TEST
    VideoEncode::set_jpg_root_path( setting.load_jpg_root_path );

    // alloc frame data.
    frame->format   =   setting.pix_fmt;
    frame->width    =   setting.width;
    frame->height   =   setting.height;

    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "get buffer fail." );

    // init sws.
    ctx->width      =   setting.width;  // 為了配合 VideoEncode 的 work around.
    ctx->height     =   setting.height;
    ctx->pix_fmt    =   setting.pix_fmt;
    VideoEncode::init_sws( setting );
    VideoEncodeHW::init_sws( setting );
#endif

    // 目前 ctx 的 time_base 可以直接拿來用. 
    // 為了方便就不另外寫一個變數存 fps 了.
    ctx->time_base  =   setting.time_base;

    // need before avcodec_open2
    if( need_global_header == true )
        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    src_width   =   setting.src_width;
    src_height  =   setting.src_height;

    //
    open_convert_demux();

    // duration per frame =  1000000 * ctx_time_base.  這邊的 time_base = 1001/24000.
    duration_per_frame  =  av_rescale( AV_TIME_BASE, ctx->time_base.num, ctx->time_base.den );
}





/*******************************************************************************
VideoEncodeHW::next_frame()

技術限制, 跟 VideoEncode 差很多
會在這邊取得 pkt, 寫入 pts, duration.

********************************************************************************/
void    VideoEncodeHW::next_frame()
{
    int ret     =   av_read_frame( demux_ctx, pkt );
    if( ret < 0 )
    {
        nv_eof  =   true;
        return;
    }

    // 不能用 get_stream_time_base.
    AVRational  stb     =   nv_stream->time_base;

	//
    AVRational  realtime_pts    =   { duration_count, AV_TIME_BASE };
    AVRational  nv_pts          =   av_div_q( realtime_pts, stb );
    pkt->pts    =   av_q2d( nv_pts );
	pkt->dts    =   pkt->pts;

	//
    AVRational  realtime_duration { duration_per_frame, AV_TIME_BASE };
    AVRational  nv_duration     =   av_div_q( realtime_duration, stb );
    pkt->duration   =   av_q2d(nv_duration);

    //
    duration_count  +=  duration_per_frame;

    if( nv_frame_count % 100 == 0 )
        MYLOG( LOG::L_DEBUG, "nv encode %d frames.", nv_frame_count )
    nv_frame_count++;
}



/*******************************************************************************
VideoEncodeHW::flush()
********************************************************************************/
int     VideoEncodeHW::flush()
{
    return  1;  // 會在 demux 階段 flush.
}




/*******************************************************************************
VideoEncodeHW::encode_timestamp()
********************************************************************************/
void    VideoEncodeHW::encode_timestamp()
{
    if( pkt == nullptr )
        MYLOG( LOG::L_WARN, "pkt is null." );
    auto ctx_tb =   nv_stream->time_base; // 理論上跟 get_timebase() 相同.
    auto stb    =   get_stream_time_base();
    av_packet_rescale_ts( pkt, ctx_tb, stb );
}




/*******************************************************************************
VideoEncodeHW::end_of_file()

這邊要小心, 因為有兩個 eof, 
一個是 frame 讀取用的, 一個是 nvenc stream 用的.
********************************************************************************/
bool    VideoEncodeHW::end_of_file()
{
    return  nv_eof;
}





/*******************************************************************************
VideoEncodeHW::get_compare_timebase()
********************************************************************************/
AVRational  VideoEncodeHW::get_compare_timebase()
{
    return  nv_stream->time_base;
}





/*******************************************************************************
VideoEncodeHW::get_pts()
********************************************************************************/
int64_t     VideoEncodeHW::get_pts()
{
    return  pkt->pts;
}





/*******************************************************************************
VideoEncodeHW::init_nv_encode()
********************************************************************************/
void    VideoEncodeHW::init_nv_encode( uint32_t width, uint32_t height, AVPixelFormat pix_fmt, VideoEncodeSetting setting )
{
    int     gpu_count   =   0;

    cuInit(0);
    cuDeviceGetCount( &gpu_count );

    if( gpu_count <= 0 )    
        MYLOG( LOG::L_ERROR, "no gpu." )       

    CUdevice    cu_device   =   0;
    cuDeviceGet( &cu_device, 0 );  // 寫死 0 表示第一張顯卡.
    
    char device_name[80];
    cuDeviceGetName( device_name, sizeof(device_name), cu_device );    
    MYLOG( LOG::L_INFO, "gpu use %s", device_name );
    cuCtxCreate( &cu_ctx, 0, cu_device );

    assert( nv_enc == nullptr );
    NV_ENC_BUFFER_FORMAT    nvenc_pix_fmt   =   NV_ENC_BUFFER_FORMAT_UNDEFINED;
    switch( pix_fmt )
    {
    case AV_PIX_FMT_YUV420P:
        nvenc_pix_fmt   =   NV_ENC_BUFFER_FORMAT_IYUV;
        break;
    case AV_PIX_FMT_YUV420P10LE:
        nvenc_pix_fmt   =   NV_ENC_BUFFER_FORMAT_YUV420_10BIT;
        break;
    default:
        MYLOG( LOG::L_ERROR, "un defined." )
    }

    // create nv_enc    
    nv_enc  =   new NvEncoderCuda ( cu_ctx, width, height, nvenc_pix_fmt );
    assert( nv_enc != nullptr );

    //
    NV_ENC_INITIALIZE_PARAMS    initialize_params   =   { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG               encode_config       =   { NV_ENC_CONFIG_VER };
    initialize_params.encodeConfig  =   &encode_config;

    NvEncoderInitParam  encode_cl_IO_options;
    //std::string parameter = "-preset default -profile main -fpsn 24000 -fpsd 1001 -gop 30 -bf 15 -rc cbr -bitrate 3M";
    //std::string parameter = "-fpsn 24000 -fpsd 1001 -gop 5 -bf 0 -rc cbr -bitrate 1M";
    char    nv_param_str[1000];
    sprintf( nv_param_str, "-preset default -profile main -fpsn %d -fpsd %d -gop %d -bf %d -rc cbr -bitrate 8M", 
                            setting.time_base.num, setting.time_base.den, setting.gop_size, setting.max_b_frames );

    //
    encode_cl_IO_options    =   NvEncoderInitParam( nv_param_str );
    nv_enc->CreateDefaultEncoderParams( &initialize_params, encode_cl_IO_options.GetEncodeGUID(), encode_cl_IO_options.GetPresetGUID() );
    encode_cl_IO_options.SetInitParams( &initialize_params, nvenc_pix_fmt );
    nv_enc->CreateEncoder( &initialize_params );

}





/*******************************************************************************
VideoEncodeHW::send_frame()

技術限制, 這邊架構跟傳統架構非常不同.
********************************************************************************/
int     VideoEncodeHW::send_frame()
{
    return  1;
}




/*******************************************************************************
VideoEncodeHW::recv_frame()

技術限制, 這邊架構跟傳統架構非常不同.
********************************************************************************/
int     VideoEncodeHW::recv_frame()
{
    if( pkt->buf != nullptr )
        return  1;
    else
        return  AVERROR(EAGAIN);
}
