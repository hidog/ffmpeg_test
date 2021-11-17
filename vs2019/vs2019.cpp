#include <iostream>

#include "player.h"


#ifdef FFMPEG_TEST
extern "C" {
#include "../ffmpeg_example/demuxing_decoding.h"
#include "../ffmpeg_example/avio_reading.h"
#include "../ffmpeg_example/metadata.h"
}
#endif


int main()
{

#if 0
    Player  player;

    player.init();
    player.play();
    player.end();
#elif defined(FFMPEG_TEST)
    //play_demuxing_decodeing();
    //avio_reader();
    metadata();
#endif

    return 0;
}
