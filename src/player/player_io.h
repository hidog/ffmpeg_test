#ifndef PLAYER_IO_H
#define PLAYER_IO_H

#include "player.h"

#if 0


// 用來輸出 frame 到 maker, 再 encode 後輸出 stream.
class PlayerIO : public Player
{
public:
    PlayerIO();
    ~PlayerIO();

    // use for live steeam
    void    init_live_stream();
    void    end_live_stream();
    void    play_live_stream();
    void    output_live_stream( Decode* dc );

    AVFrame*    get_new_v_frame();
    AVFrame*    get_new_a_frame();

    std::function< void(AVFrame*) >     add_audio_frame_cb;
    std::function< void(AVFrame*) >     add_video_frame_cb;

private:
};


#endif

#endif