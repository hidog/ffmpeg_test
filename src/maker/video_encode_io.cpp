#include "video_encode_io.h"

#include "tool.h"


extern "C" {

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>

} // end extern "C"



/*******************************************************************************
VideoEncodeIO::VideoEncodeIO()
********************************************************************************/
VideoEncodeIO::VideoEncodeIO()
{}



/*******************************************************************************
VideoEncodeIO::~VideoEncodeIO()
********************************************************************************/
VideoEncodeIO::~VideoEncodeIO()
{}




/*******************************************************************************
VideoEncodeIO::next_frame()
********************************************************************************/
void        VideoEncodeIO::next_frame()
{
    frame   =   encode::get_video_frame();
}






/*******************************************************************************
VideoEncodeIO::init


********************************************************************************/
void  VideoEncodeIO::init( int st_idx, VideoEncodeSetting setting, bool need_global_header )
{
    //
    Encode::init( st_idx, setting.code_id, false );
    init_base( setting, need_global_header );

    // ┏把计ゼ秨场砞竚,ぇσ璶ぃ璶秨    
    ctx->bit_rate   =   3000000;
    
    assert( codec->id == AV_CODEC_ID_H264 || codec->id == AV_CODEC_ID_H265 );
    
    av_opt_set( ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set( ctx->priv_data, "tune", "zerolatency", 0);

    //if( ctx->codec_id == AV_CODEC_ID_H264 )
      //  av_opt_set( ctx->priv_data, "x264-params", "sliced-threads=10", 0);   // 北encode thread

    ctx->thread_count   =   10;

    //
    int     ret     =   avcodec_open2( ctx, codec, nullptr );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "open fail" );
}
