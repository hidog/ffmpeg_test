#include "translate.h"

#include <QString>
#include "player/player.h"




/*******************************************************************************
translate_media_file
********************************************************************************/
void    translate_media_file()
{
    std::string     input_file  =   "H:/1.mkv";
    std::string     audio_file  =   "H:/a.data";

    extraction_audio( input_file, audio_file );

}




/*******************************************************************************
extraction_audio
********************************************************************************/
void    extraction_audio( std::string input_file, std::string audio_file )
{
#ifdef FFMPEG_TEST
    DecodeSetting   setting;
    setting.io_type     =   IO_Type::DEFAULT;
    setting.filename    =   input_file;    

    Player  player;  

    player.set( setting );
    player.init();

    player.set_output_audio_pcm_path( audio_file );

    player.play_output_audio();
    player.end();
#endif
}
