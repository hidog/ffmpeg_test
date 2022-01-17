#include "maker_io.h"

#include "mux.h"
#include "tool.h"

#include <thread>






/*******************************************************************************
MakerIO::MakerIO()
********************************************************************************/
MakerIO::MakerIO()
    :   Maker()
{}
  


/*******************************************************************************
MakerIO::~MakerIO()
********************************************************************************/
MakerIO::~MakerIO()
{}




/*******************************************************************************
Maker::release_encode_video_frame()
********************************************************************************/
void    MakerIO::release_encode_video_frame( AVFrame *vf )
{
#if 0
    av_frame_free( &vf );
#endif
}




/*******************************************************************************
Maker::release_encode_audio_frame()
********************************************************************************/
void    MakerIO::release_encode_audio_frame( AVFrame *af )
{
#if 0
    av_frame_free( &af );
#endif
}





/*******************************************************************************
io_write_data
********************************************************************************/
int     io_write_data( void *opaque, uint8_t *buf, int buf_size )
{
    return  1;

#if 0
    InputOutput*    io  =   (InputOutput*)opaque;
    int     ret     =   io->write( buf, buf_size );
    return  ret;
#endif
}







/*******************************************************************************
MakerIO::work_live_stream()
********************************************************************************/
void    MakerIO::work()
{
    // wait for data to start.
    while( encode::audio_need_wait() == true || encode::video_need_wait() == true )
        SLEEP_10MS;    

    
    muxer->write_header();

    // note: stream_time_base 會在 write header 後改變值, 所以需要再 write header 後做設置.
    AVRational  stb;
    stb     =   muxer->get_video_stream_timebase();
    v_encoder.set_stream_time_base(stb);
    stb     =   muxer->get_audio_stream_timebase();
    a_encoder.set_stream_time_base(stb);

    Maker::work_without_subtitle();
}










/*******************************************************************************
MakerIO::init()
********************************************************************************/
void    MakerIO::init( EncodeSetting _setting, VideoEncodeSetting v_setting, AudioEncodeSetting a_setting )
{
    setting =   _setting;

    init_muxer();

    bool    need_global_header  =   muxer->is_need_global_header();

    v_encoder.init( default_video_stream_index, v_setting, need_global_header );
    a_encoder.init( default_audio_stream_index, a_setting, need_global_header );

    //
    auto v_ctx  =   v_encoder.get_ctx();
    auto a_ctx  =   a_encoder.get_ctx();
    auto s_ctx  =   s_encoder.get_ctx();

    muxer->open( setting, v_ctx, a_ctx, s_ctx );
}


