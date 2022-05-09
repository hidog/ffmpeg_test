#ifndef PLAYER_STREAM_H
#define PLAYER_STREAM_H

#include "player.h"


/* 
    用來輸出 frame 到 maker, 再 encode 後輸出 stream.
*/


class DLL_API PlayerStream : public Player
{
public:
    PlayerStream();
    ~PlayerStream();

    PlayerStream( const PlayerStream& ) = delete;
    PlayerStream( PlayerStream&& ) = delete;

    PlayerStream& operator = ( const PlayerStream& ) = delete;
    PlayerStream& operator = ( PlayerStream&& ) = delete;

    // use for live steeam
    void    play_QT() override;
    int     init() override;
    int     end() override;

    void    output_live_stream( Decode* dc );

    //AVFrame*    get_new_v_frame(); // move to Player, for translate media file.
    AVFrame*    get_new_a_frame();

    std::function< void(AVFrame*) >     add_audio_frame_cb;
    //std::function< void(AVFrame*) >     add_video_frame_cb; // move to Player, for translate media file. 

private:

    // use for live stream.
    bool        is_live_stream      =   false;
    int64_t     audio_pts_count     =   0;

};




#endif