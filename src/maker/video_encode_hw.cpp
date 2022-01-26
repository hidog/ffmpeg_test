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
{}





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

    if( eof_flag == true )
    {
        nv_enc->EndEncode(vPacket);
    }
    else
    {
        int w = nv_enc->GetEncodeWidth();
        int h = nv_enc->GetEncodeHeight();

        int nFrameSize = nv_enc->GetFrameSize();
        uint8_t *ptr = new uint8_t[nFrameSize];

        //
        av_image_fill_pointers( frame->data, AV_PIX_FMT_YUV420P, 720, video_data[0], video_linesize );

        NvEncoderCuda::CopyToDeviceFrame( cuContext, video_data[0], 0, (CUdeviceptr)nv_input_frame->inputPtr,
                                          (int)nv_input_frame->pitch,
                                          w, h, 
                                          CU_MEMORYTYPE_HOST, 
                                          nv_input_frame->bufferFormat,
                                          nv_input_frame->chromaOffsets,
                                          nv_input_frame->numChromaPlanes );
        nv_enc->EncodeFrame(vPacket);
        delete [] ptr;
    }

    if( vPacket.size() > 0 )    
    {
        for( int i = 0; i < vPacket.size(); i++ )
        {
            BufferData item;
            item.data = vPacket[i];
            item.read_size = 0;
            item.remain_size = vPacket[i].size();
            
            buffer_list.emplace_back( item );
        }
    }

    if( buffer_list.size() == 0 && eof_flag == true )
        return EOF;
    if( buffer_list.size() == 0 )
        return  0;
    else
    {
        while( true )
        {
            BufferData &item = buffer_list.front();
            if( item.remain_size <= size )
            {
                memcpy( buffer, item.data.data() + item.read_size, item.remain_size );
                int read_size = item.remain_size;
                buffer_list.pop_front();
                return read_size;
            }
            else
            {
                memcpy( buffer, item.data.data() + item.read_size, size );
                item.read_size += size;
                item.remain_size -= size;
                return size;
            }
        }
    }


    return 0;
}



/*******************************************************************************
VideoEncodeHW::open_convert_ctx()
********************************************************************************/
void    VideoEncodeHW::open_convert_ctx()
{
    //      
    int  ret    =   0;
    fmt_ctx     =   avformat_alloc_context();    
	
    uint8_t     *input_buf   =   (uint8_t*)av_malloc(4096);
    assert( input_buf != nullptr );

	AVIOContext     *io_ctx =   avio_alloc_context( input_buf, 4096, 0, (void*)this, demux_read, nullptr, nullptr );
    assert( io_ctx != nullptr );

	AVInputFormat   *input_fmt   =   nullptr;
    ret         =   av_probe_input_buffer( io_ctx, &input_fmt, nullptr, nullptr, 0, 0 );
    assert( ret == 0 );
	fmt_ctx->pb =   io_ctx;
    ret         =   avformat_open_input( &fmt_ctx, nullptr, input_fmt, nullptr );
    assert( ret == 0 );

    fmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;  

    if( ret < 0 )
        MYLOG( LOG::L_ERROR, "Could not open" );

    //
    ret     =   avformat_find_stream_info( fmt_ctx, nullptr );
    if( ret < 0) 
        MYLOG( LOG::L_ERROR, "Could not find stream information. ret = %d", ret );

    stream = fmt_ctx->streams[0];
    printf( "stream time base = %d %d\n", stream->time_base.num, stream->time_base.den );

    ret     =   avcodec_parameters_to_context( ctx, stream->codecpar );

}




/*******************************************************************************
VideoEncodeHW::init()
********************************************************************************/
void    VideoEncodeHW::init( int st_idx, VideoEncodeSetting setting, bool need_global_header )
{
    int ret = 0;
    
    Encode::init( st_idx, setting.code_id );

#ifdef FFMPEG_TEST
    VideoEncode::set_jpg_root_path( setting.load_jpg_root_path );

    // frame setting
    frame->format   =   setting.pix_fmt;
    frame->width    =   setting.width;
    frame->height   =   setting.height;

    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "get buffer fail." );

    // data for sws.
    video_bufsize   =   av_image_alloc( video_data, video_linesize, setting.width, setting.height, setting.pix_fmt, 1 );

    sws_ctx     =   sws_getContext( setting.src_width, setting.src_height, setting.src_pix_fmt,    // src
                                    setting.width,        setting.height,        setting.pix_fmt,           // dst
                                    SWS_BICUBIC, NULL, NULL, NULL );
#endif

    //
    ctx->time_base.num = 24000;
    ctx->time_base.den = 1001;

    // need before avcodec_open2
    if( need_global_header == true )
        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    src_width   =   setting.src_width;
    src_height  =   setting.src_height;

    init_nv_encode( setting.width, setting.height, setting.pix_fmt );

    open_convert_ctx();
}




/*******************************************************************************
VideoEncodeHW::next_frame()
********************************************************************************/
void    VideoEncodeHW::next_frame()
{
    int ret = av_read_frame( fmt_ctx, pkt );

    if( ret < 0 )
    {
        nv_eof = true;
        return;
    }

    // 不能用 get_stream_time_base.
    AVRational stb = stream->time_base;

    // dpf = duration per frame = 1000000 / ctx_time_base, 用 av_rescale 做計算, 所以 den, num 順序反過來.
    int64_t dpf =  av_rescale( AV_TIME_BASE, ctx->time_base.den, ctx->time_base.num );  

	//
    AVRational tmp_1 = { decode_count*dpf, AV_TIME_BASE };
    AVRational tmp_2 = av_div_q( tmp_1, stb );

    pkt->pts = av_q2d( tmp_2 );  // decode_count*dpf 考慮用變數來累加.
	pkt->dts = pkt->pts;

	//
    AVRational tmp_r { dpf, AV_TIME_BASE };
    AVRational tmp_r2 = av_div_q( tmp_r, stb );
    pkt->duration = av_q2d(tmp_r2);

    decode_count++;
}



/*******************************************************************************
VideoEncodeHW::encode_timestamp()
********************************************************************************/
void    VideoEncodeHW::encode_timestamp()
{
    if( pkt == nullptr )
        MYLOG( LOG::L_WARN, "pkt is null." );
    auto ctx_tb =   stream->time_base; //get_timebase();
    auto stb    =   get_stream_time_base();
    av_packet_rescale_ts( pkt, ctx_tb, stb );
}



/*******************************************************************************
VideoEncodeHW::end_of_file()
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
    return  stream->time_base;
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
void    VideoEncodeHW::init_nv_encode( uint32_t width, uint32_t height, AVPixelFormat pix_fmt )
{
    cuInit(0);
    int nGpu = 0;
    cuDeviceGetCount(&nGpu);

    if( nGpu <= 0 )    
        MYLOG( LOG::L_ERROR, "no gpu." )       

    CUdevice cuDevice = 0;
    cuDeviceGet( &cuDevice, 0 );  // 寫死 0 表示第一張顯卡.
    
    char szDeviceName[80];
    cuDeviceGetName( szDeviceName, sizeof(szDeviceName), cuDevice );    
    MYLOG( LOG::L_INFO, "gpu use %s", szDeviceName );

    //CUcontext cuContext = NULL;
    cuCtxCreate( &cuContext, 0, cuDevice );

    assert( nv_enc == nullptr );

    NV_ENC_BUFFER_FORMAT    eFormat    =   NV_ENC_BUFFER_FORMAT_UNDEFINED;
    switch( pix_fmt )
    {
    case AV_PIX_FMT_YUV420P:
        eFormat     =   NV_ENC_BUFFER_FORMAT_IYUV;
        break;
    case AV_PIX_FMT_YUV420P10LE:
        eFormat     =   NV_ENC_BUFFER_FORMAT_YUV420_10BIT;
        break;
    default:
        MYLOG( LOG::L_ERROR, "un defined." )
    }

    // create nv_enc    
    nv_enc  =   new NvEncoderCuda { cuContext, width, height, eFormat };
    assert( nv_enc != nullptr );

    //
    NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
    initializeParams.encodeConfig = &encodeConfig;

    NvEncoderInitParam encodeCLIOptions;
    //std::string parameter = "-preset default -profile main -fpsn 24000 -fpsd 1001 -gop 30 -bf 15 -rc cbr -bitrate 3M";
    std::string parameter = "-fpsn 24000 -fpsd 1001 -gop 5 -bf 0 -rc cbr -bitrate 1M";

    encodeCLIOptions = NvEncoderInitParam( parameter.c_str() );

    nv_enc->CreateDefaultEncoderParams( &initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID() );
    encodeCLIOptions.SetInitParams( &initializeParams, eFormat );

    nv_enc->CreateEncoder( &initializeParams );

    
    int nFrameSize = nv_enc->GetFrameSize();
    MYLOG( LOG::L_DEBUG, "frame size = %d", nFrameSize );

}





/*******************************************************************************
VideoEncodeHW::send_frame()
********************************************************************************/
int     VideoEncodeHW::send_frame()
{
    return 1;
}




/*******************************************************************************
VideoEncodeHW::send_frame()
********************************************************************************/
int     VideoEncodeHW::recv_frame()
{
    if( pkt->buf != nullptr )
        return 1;
    else
        return AVERROR(EAGAIN);
}
