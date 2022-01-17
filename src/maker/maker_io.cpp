#include "maker_io.h"

#include "mux_io.h"
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
{
    end();
}




/*******************************************************************************
MakerIO::end()
********************************************************************************/
void    MakerIO::end()
{
    if( IO != nullptr )
    {
        while( IO->is_stop() == false )
            SLEEP_10MS;
        delete IO;
        IO  =   nullptr;
    }

    Maker::end();
}




/*******************************************************************************
MakerIO::is_connect()
********************************************************************************/
bool    MakerIO::is_connect()
{
    if( IO == nullptr )
        return  false;
    bool    flag    =   IO->is_connect();
    return  flag;
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
MakerIO::init_IO()
********************************************************************************/
void    MakerIO::init_IO()
{
    assert( setting.io_type != IO_Type::DEFAULT );  // defulat use for mux.
    assert( IO == nullptr );
    IO  =   create_IO( setting.io_type, IO_Direction::SEND );
    IO->set_encode( setting );
    IO->init();
}





/*******************************************************************************
MakerIO::init()
********************************************************************************/
void    MakerIO::init( EncodeSetting _setting, VideoEncodeSetting v_setting, AudioEncodeSetting a_setting )
{
    setting =   _setting;
    init_IO();  // need call after set setting.

    //
    muxer   =   new MuxIO;
    muxer->init( setting );

    bool    need_global_header  =   muxer->is_need_global_header();

    v_encoder.init( default_video_stream_index, v_setting, need_global_header );
    a_encoder.init( default_audio_stream_index, a_setting, need_global_header );

    //
    auto v_ctx  =   v_encoder.get_ctx();
    auto a_ctx  =   a_encoder.get_ctx();

    IO->open();
    
    MuxIO*  muxer_io    =   dynamic_cast<MuxIO*>(muxer);
    if( muxer_io == nullptr )
        MYLOG( LOG::ERROR, "convert mux to mux_io fail." );

    muxer_io->open( setting, v_ctx, a_ctx, IO );
}





