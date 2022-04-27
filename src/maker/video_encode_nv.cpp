#include "video_encode_nv.h"
#include "tool.h"

#include "hw/NvEncoderCuda.h"
#include "hw/NvEncoderCLIOptions.h"



extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

} // extern "C"







/*******************************************************************************
VideoEncodeNV::VideoEncodeNV()
********************************************************************************/
VideoEncodeNV::VideoEncodeNV()
    :   VideoEncode()
{}




/*******************************************************************************
VideoEncodeNV::~VideoEncodeNV()
********************************************************************************/
VideoEncodeNV::~VideoEncodeNV()
{
    end();
}




/*******************************************************************************
VideoEncodeNV::end()
********************************************************************************/
void    VideoEncodeNV::end()
{
    VideoEncode::end();

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
}





/*******************************************************************************
io_read_data
********************************************************************************/
int     demux_read( void *opaque, uint8_t *buffer, int size )
{
    VideoEncodeNV   *video_enc_nv   =   (VideoEncodeNV*)opaque;
    assert( video_enc_nv != nullptr );

    int     read_size   =   0;
    
    while( true )
    {
        read_size   =   video_enc_nv->get_nv_encode_data( buffer, size );
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
VideoEncodeNV::get_nv_encode_data
********************************************************************************/
int     VideoEncodeNV::get_nv_encode_data( uint8_t *buffer, int size )
{
    // �ϥ� VideoEncode ����Ū�� frame, �A�� nvenc encode.
    VideoEncode::next_frame();

    // 
    const NvEncInputFrame   *nv_input_frame     =   nv_enc->GetNextInputFrame();
    if( nv_input_frame == nullptr )
        MYLOG( LOG::L_ERROR, "nv_input_frame is nullptr." )

    /*
        NOTE: NvEncodeHW �]���Y�ǲz��, �ݭn��� eof
        �@�ӬOŪ�� frame �� eof.
        �@�ӬO stream ������ eof.
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

    // get encoded data.
    if( encoded_vec.size() > 0 )    
    {
        for( int i = 0; i < encoded_vec.size(); i++ )
        {
            NvEncBuffer     item;
            item.enc_data       =   std::move( encoded_vec[i] );
            item.read_size      =   0;
            item.remain_size    =   item.enc_data.size();            
            nv_encoded_list.emplace_back( item );
        }
    }

    int     buf_read_size       =   0;     // �~�� buffer �ϥ�
    int     buf_remain_size     =   size;

    if( nv_encoded_list.size() == 0 && eof_flag == true )
        return  EOF;
    if( nv_encoded_list.size() == 0 )
        return  0; 
    else
    {
        while( true )
        {
            NvEncBuffer &item   =   nv_encoded_list.front();
            if( item.remain_size <= buf_remain_size )
            {
                memcpy( buffer + buf_read_size, item.enc_data.data() + item.read_size, item.remain_size );
                buf_read_size   +=  item.remain_size;
                buf_remain_size -=  item.remain_size;
                nv_encoded_list.pop_front();
            }
            else
            {
                int     tmp_size    =   buf_remain_size; // �Ȧs, �קK�]������ read_size, remain_size �SŪ���ȳy���� bug.

                memcpy( buffer + buf_read_size, item.enc_data.data() + item.read_size, tmp_size );
                item.read_size      +=  tmp_size;
                item.remain_size    -=  tmp_size;
                buf_read_size       +=  tmp_size;
                buf_remain_size     -=  tmp_size;
            }

            if( buf_remain_size <= 0 || nv_encoded_list.size() == 0 )
                break;
        }
    }


    return  buf_read_size;
}






/*******************************************************************************
VideoEncodeNV::open_convert_demux()

nvenc �X�Ӫ� stream �� demux �ѥX packet, �[�W pts, duration, �A��J mux.
********************************************************************************/
int    VideoEncodeNV::open_convert_demux()
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

	const AVInputFormat   *input_fmt  =   nullptr;
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

    // nv_stream �|�Φb mux ���q.
    nv_stream   =   demux_ctx->streams[0];  // �z�פW�u�|���@�� stream, �ӥB�O video stream.
    assert( nv_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO );

    // �ݭn�N��T copy �i�h ctx. mux ���ɭԷ|�A�q ctx Ū��.
    // �o�䪺 ctx �u�ΨӦs��T, ���|���� encode.
    ret     =   avcodec_parameters_to_context( ctx, nv_stream->codecpar );
    assert( ret >= 0 );

    // note: �n encode 10bit ���ɭ�, ������ʱN pix_fmt �令 P010, ���M�|�L�k���� 
    if( ctx->pix_fmt == AV_PIX_FMT_YUV420P10LE )
        ctx->pix_fmt = AV_PIX_FMT_P010LE;
}





/*******************************************************************************
VideoEncodeNV::init_nv_frame_buffer()
********************************************************************************/
void    VideoEncodeNV::init_nv_frame_buffer( VideoEncodeSetting setting )
{
    if( nv_enc == nullptr )
        MYLOG( LOG::L_ERROR, "nv_enc not init." )

    nv_bufsize  =   av_image_alloc( nv_data, nv_linesize, setting.width, setting.height, setting.pix_fmt, 1 );
}






/*******************************************************************************
VideoEncodeNV::init()
********************************************************************************/
void    VideoEncodeNV::init( int st_idx, VideoEncodeSetting setting, bool need_global_header )
{
    VideoEncodeSetting  tmp_setting     =   setting;  // �]�� yuvp10 ���ɭԻݭn�ק� pix_fmt. �Τ@�ӼȦs�ܼ�,�קK��������l setting.

    if( tmp_setting.pix_fmt == AV_PIX_FMT_YUV420P )
        tmp_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;   // ��ڤW�O do nothing.
    else if( tmp_setting.pix_fmt == AV_PIX_FMT_YUV420P10LE )
        tmp_setting.pix_fmt   =   AV_PIX_FMT_P010LE;  // yuv420p10 �n�ন yuv420p16. nvidia ��ڤW�O�惡�榡���ާ@.
    else
        assert(0); // un support.

    int     ret     =   0;
    
    Encode::init( st_idx, tmp_setting.code_id );
    init_nv_encode( tmp_setting.width, tmp_setting.height, setting.pix_fmt, tmp_setting );  // ���涶�Ǥ����H�K��, ���̩ۨ�.
                                                      //   ^^^^^^^^^^^^^^^  �d�N���ƭn���ª� setting

#ifdef FFMPEG_TEST
    VideoEncode::set_jpg_root_path( tmp_setting.load_jpg_root_path );

    // alloc frame data.
    frame->format   =   tmp_setting.pix_fmt;
    frame->width    =   tmp_setting.width;
    frame->height   =   tmp_setting.height;

    ret     =   av_frame_get_buffer( frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::L_ERROR, "get buffer fail." );

    // init sws.
    ctx->pix_fmt    =   tmp_setting.pix_fmt;
    ctx->width      =   tmp_setting.width;  // ���F�t�X VideoEncode �� work around.
    ctx->height     =   tmp_setting.height;

    VideoEncode::init_sws( tmp_setting );
    init_nv_frame_buffer( tmp_setting );
#endif

    // �ثe ctx �� time_base �i�H�������ӥ�. 
    // ���F��K�N���t�~�g�@���ܼƦs fps �F.
    ctx->time_base  =   tmp_setting.time_base;

    // need before avcodec_open2
    if( need_global_header == true )
        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    src_width   =   tmp_setting.src_width;
    src_height  =   tmp_setting.src_height;

    //
    open_convert_demux();

    // duration per frame =  1000000 * ctx_time_base.  �o�䪺 time_base = 1001/24000.
    duration_per_frame  =  av_rescale( AV_TIME_BASE, ctx->time_base.num, ctx->time_base.den );
}





/*******************************************************************************
VideoEncodeNV::next_frame()

�޳N����, �� VideoEncode �t�ܦh
�|�b�o����o pkt, �g�J pts, duration.

********************************************************************************/
void    VideoEncodeNV::next_frame()
{
    int ret     =   av_read_frame( demux_ctx, pkt );
    if( ret < 0 )
    {
        nv_eof  =   true;
        return;
    }

    // ����� get_stream_time_base.
    AVRational  stb     =   nv_stream->time_base;

	//
#if 0
    // �o��|�o�� overflow, �ҥH���g�k
    AVRational  realtime_pts    =   { duration_count, AV_TIME_BASE };
    AVRational  nv_pts          =   av_div_q( realtime_pts, stb );
    pkt->pts    =   av_q2d( nv_pts );
#else
    pkt->pts    =   1.0 / AV_TIME_BASE * duration_count * stb.den / stb.num;
#endif
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
VideoEncodeNV::flush()
********************************************************************************/
int     VideoEncodeNV::flush()
{
    return  1;  // �|�b demux ���q flush.
}




/*******************************************************************************
VideoEncodeNV::encode_timestamp()
********************************************************************************/
void    VideoEncodeNV::encode_timestamp()
{
    if( pkt == nullptr )
        MYLOG( LOG::L_WARN, "pkt is null." );
    auto ctx_tb =   nv_stream->time_base; // �z�פW�� get_timebase() �ۦP.
    auto stb    =   get_stream_time_base();
    av_packet_rescale_ts( pkt, ctx_tb, stb );
}




/*******************************************************************************
VideoEncodeNV::end_of_file()

�o��n�p��, �]������� eof, 
�@�ӬO frame Ū���Ϊ�, �@�ӬO nvenc stream �Ϊ�.
********************************************************************************/
bool    VideoEncodeNV::end_of_file()
{
    return  nv_eof;
}





/*******************************************************************************
VideoEncodeNV::get_compare_timebase()
********************************************************************************/
AVRational  VideoEncodeNV::get_compare_timebase()
{
    return  nv_stream->time_base;
}





/*******************************************************************************
VideoEncodeNV::get_pts()
********************************************************************************/
int64_t     VideoEncodeNV::get_pts()
{
    return  pkt->pts;
}





/*******************************************************************************
VideoEncodeNV::init_nv_encode()
********************************************************************************/
void    VideoEncodeNV::init_nv_encode( uint32_t width, uint32_t height, AVPixelFormat pix_fmt, VideoEncodeSetting setting )
{
    int     gpu_count   =   0;

    cuInit(0);
    cuDeviceGetCount( &gpu_count );

    if( gpu_count <= 0 )    
        MYLOG( LOG::L_ERROR, "no gpu." )       

    CUdevice    cu_device   =   0;
    cuDeviceGet( &cu_device, 0 );  // �g�� 0 ��ܲĤ@�i��d.
    
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

    // note: �H�ᦳ�ݭn���ܦA�W�[�X�R��. ��²��g.
    char    nv_param_str[1000];
    if( setting.code_id == AV_CODEC_ID_H265 )
    {
        sprintf( nv_param_str, "-codec hevc -preset default -profile main -fpsn %d -fpsd %d -gop %d -bf %d -rc cbr -bitrate 3M", 
                                setting.time_base.num, setting.time_base.den, setting.gop_size, setting.max_b_frames );
    }
    else if( setting.code_id == AV_CODEC_ID_H264 )
    {
        if( setting.pix_fmt == AV_PIX_FMT_YUV420P10LE )
            MYLOG( LOG::L_ERROR, "not support." )
        sprintf( nv_param_str, "-codec h264 -preset default -profile main -fpsn %d -fpsd %d -gop %d -bf %d -rc cbr -bitrate 3M", 
                                setting.time_base.num, setting.time_base.den, setting.gop_size, setting.max_b_frames );
    }

    //
    encode_cl_IO_options    =   NvEncoderInitParam( nv_param_str );
    nv_enc->CreateDefaultEncoderParams( &initialize_params, encode_cl_IO_options.GetEncodeGUID(), encode_cl_IO_options.GetPresetGUID() );
    encode_cl_IO_options.SetInitParams( &initialize_params, nvenc_pix_fmt );
    nv_enc->CreateEncoder( &initialize_params );
}





/*******************************************************************************
VideoEncodeNV::send_frame()

�޳N����, �o��[�c��ǲά[�c�D�`���P.
********************************************************************************/
int     VideoEncodeNV::send_frame()
{
    return  1;
}




/*******************************************************************************
VideoEncodeNV::recv_frame()

�޳N����, �o��[�c��ǲά[�c�D�`���P.
********************************************************************************/
int     VideoEncodeNV::recv_frame()
{
    if( pkt->buf != nullptr )
        return  1;
    else
        return  AVERROR(EAGAIN);
}
