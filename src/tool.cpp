#include "tool.h"
#include "player/play_def.h"

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
audio_need_wait
********************************************************************************/
bool    audio_need_wait()
{
    return  aframe_queue.size() <= ENCODE_WAIT_SIZE;
}


/*******************************************************************************
video_need_wait
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





/*******************************************************************************
is_audio_queue_full
*******************************************************************************/
bool    is_audio_queue_full()
{
    if( aframe_queue.size() > MAX_QUEUE_SIZE )
    {
        MYLOG( LOG::L_WARN, "aframe queue full." );
        return  true;
    }
    else
        return  false;
}





/*******************************************************************************
is_video_queue_full
*******************************************************************************/
bool    is_video_queue_full()
{
    if( vframe_queue.size() > MAX_QUEUE_SIZE )
    {
        MYLOG( LOG::L_WARN, "vframe queue full." );
        return  true;
    }
    else
        return  false;
}





/*******************************************************************************
get_video_size
*******************************************************************************/
int     get_video_size()
{
    return  vframe_queue.size();
}




} // end namespace encode






namespace decode {
    
std::queue<AudioData>    audio_queue;
std::queue<VideoData>    video_queue;

std::mutex  a_mtx;
std::mutex  v_mtx;

// 在seek事件的時候,負責跟UI的video worker, audio worker做同步
// 簡易做法, 有空再修
bool ui_v_seek_lock  =   false;  
bool ui_a_seek_lock  =   false;


/*******************************************************************************
get_v_seek_lock()
********************************************************************************/
bool&   get_v_seek_lock() 
{ 
    return ui_v_seek_lock; 
}


/*******************************************************************************
get_a_seek_lock()
********************************************************************************/
bool&   get_a_seek_lock() 
{
    return ui_a_seek_lock; 
}


/*******************************************************************************
get_audio_size()
********************************************************************************/
int     get_audio_size()
{
    return  audio_queue.size();
}


/*******************************************************************************
get_video_size()
********************************************************************************/
int     get_video_size()
{
    return  video_queue.size();
}



/*******************************************************************************
add_audio_data()
********************************************************************************/
void    add_audio_data( AudioData a_data )
{
    std::lock_guard<std::mutex> locker(a_mtx);
    audio_queue.push(a_data);
}




/*******************************************************************************
add_video_data()
********************************************************************************/
void    add_video_data( VideoData v_data )
{
    std::lock_guard<std::mutex> locker(v_mtx);
    video_queue.push(v_data);
}




/*******************************************************************************
get_audio_data()
********************************************************************************/
AudioData    get_audio_data()
{
    a_mtx.lock();
    AudioData   ad  =   audio_queue.front();       
    audio_queue.pop();
    a_mtx.unlock();

    return  ad;
}




/*******************************************************************************
get_video_data()
********************************************************************************/
VideoData    get_video_data()
{
    v_mtx.lock();
    VideoData vd    =   video_queue.front();
    video_queue.pop();
    v_mtx.unlock();
    return  vd;
}




/*******************************************************************************
clear_audio_queue()
********************************************************************************/
void    clear_audio_queue()
{
    std::lock_guard<std::mutex> locker(a_mtx);

    AudioData   a_data;
    while( audio_queue.empty() == false )
    {
        a_data   =   audio_queue.front();
        delete [] a_data.pcm;
        audio_queue.pop();
    }
}




/*******************************************************************************
clear_video_queue()
********************************************************************************/
void    clear_video_queue()
{
    std::lock_guard<std::mutex> locker(v_mtx);

    while( video_queue.empty() == false )
        video_queue.pop();
}


} // end namespace decode

