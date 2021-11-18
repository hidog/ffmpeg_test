#include <iostream>

#include "player.h"



/*
http://dranger.com/ffmpeg/tutorial05.html
http://dranger.com/ffmpeg/tutorial01.html
*/



int main()
{
    Player  player;

    player.init();
    player.play();
    player.end();

    return 0;
}
