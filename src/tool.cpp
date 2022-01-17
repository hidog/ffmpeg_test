#include "tool.h"

#include <queue>
#include <mutex>




// note: 以後如果需要 multi-encode, 需要寫一個 manager.
namespace encode {

bool    is_finish   =   false;

std::queue<AVFrame*>     aframe_queue;
std::queue<AVFrame*>     vframe_queue;

std::mutex   af_mtx;
std::mutex   vf_mtx;





/*******************************************************************************
set_is_finish
********************************************************************************/
void    set_is_finish( bool flag )
{
    is_finish   =   flag;
}



/*******************************************************************************
audio_is_empty
********************************************************************************/
bool    audio_need_wait()
{
    return  aframe_queue.size() <= ENCODE_WAIT_SIZE;
}


/*******************************************************************************
audio_is_empty
********************************************************************************/
bool    video_need_wait()
{
    return  vframe_queue.size() <= ENCODE_WAIT_SIZE;
}


/*******************************************************************************
add_audio_frame
********************************************************************************/
void    add_audio_frame( AVFrame* af )
{
    const std::lock_guard<std::mutex> lock(af_mtx);
    aframe_queue.emplace(af);
}



/*******************************************************************************
add_video_frame
********************************************************************************/
void    add_video_frame( AVFrame* vf )
{
    const std::lock_guard<std::mutex> lock(vf_mtx);
    vframe_queue.emplace(vf);
}



/*******************************************************************************
get_audio_frame
********************************************************************************/
AVFrame*    get_audio_frame()
{
    while( aframe_queue.empty() == true && is_finish == false )
        SLEEP_1MS;

    if( aframe_queue.empty() == true && is_finish == true )
        return  nullptr;

    AVFrame*    af  =   aframe_queue.front();

    af_mtx.lock();
    aframe_queue.pop();
    af_mtx.unlock();

    return  af;
}



/*******************************************************************************
get_video_frame
*******************************************************************************/
AVFrame*    get_video_frame()
{
    while( vframe_queue.empty() == true && is_finish == false )
        SLEEP_1MS;

    if( vframe_queue.empty() == true && is_finish == true )
        return  nullptr;

    AVFrame*    vf  =   vframe_queue.front();

    vf_mtx.lock();
    vframe_queue.pop();
    vf_mtx.unlock();

    return  vf;
}


} // end namespace encode
