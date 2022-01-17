#include "audio_encode_io.h"

#include "tool.h"

extern "C" {

#include <libavcodec/avcodec.h>

} // end extern "C"



/*******************************************************************************
AudioEncodeIO::AudioEncodeIO()
********************************************************************************/
AudioEncodeIO::AudioEncodeIO()
    :   AudioEncode()
{}







/*******************************************************************************
AudioEncodeIO::~AudioEncodeIO()
********************************************************************************/
AudioEncodeIO::~AudioEncodeIO()
{}






/*******************************************************************************
AudioEncodeIO::next_frame()
********************************************************************************/
void        AudioEncodeIO::next_frame()
{
    frame   =   encode::get_audio_frame();
}






/*******************************************************************************
AudioEncodeIO::init()
********************************************************************************/
void    AudioEncodeIO::init( int st_idx, AudioEncodeSetting setting, bool need_global_header )
{
    Encode::init( st_idx, setting.code_id, false );
    init_base( setting, need_global_header );

    int     ret     =   avcodec_open2( ctx, codec, nullptr );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "open fail." );
}
