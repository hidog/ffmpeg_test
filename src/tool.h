#ifndef TOOL_H
#define TOOL_H

#include <cassert>
#include <QImage>

#include "player/play_def.h"

//#define USE_MT


constexpr int R_ERROR       =   -1;  // R_ERROR 跟太多檔案起衝突, 才故意改掉命名.
constexpr int R_SUCCESS     =   0;
constexpr int HAVE_FRAME    =   1; // 需要用 1 來判斷是否還有 frame.

       
#define SLEEP_10MS std::this_thread::sleep_for( std::chrono::milliseconds(10) )
#define SLEEP_1MS std::this_thread::sleep_for( std::chrono::milliseconds(1) )


enum class LOG
{
    L_DEBUG = 0,
    L_INFO,
    L_WARN,
    L_ERROR,        // R_ERROR 太容易撞名 才改名稱
};



#define MYLOG( TAG, ... ) \
	{ \
        if( TAG == LOG::L_DEBUG ) \
		    printf("[DEBUG] [%s] [%d] ", __FILE__, __LINE__); \
        else if( TAG == LOG::L_INFO ) \
            printf("[INFO] [%s] [%d] ", __FILE__, __LINE__); \
        else if( TAG == LOG::L_WARN ) \
            printf("[WARN] [%s] [%d] ", __FILE__, __LINE__); \
        else \
            printf("[ERR] [%s] [%d] ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
        if( TAG == LOG::L_ERROR ) \
            assert(0); \
	}




#ifdef FFMPEG_TEST
#define DLL_API
#elif defined(_BUILD_DLL)
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif







struct MediaInfo
{
    // video
    int     width;
    int     height;
    int     time_num;
    int     time_den;
    int     pix_fmt;

    // audio
    int     channel_layout;
    int     sample_rate;
    int     sample_fmt;
};




struct AVFrame;


// push frame to queue, and use for encode.
// NOTE: 如果要支援 multi-encode, 需要改寫一個 manager.
namespace encode {

constexpr int   ENCODE_WAIT_SIZE    =   20;
constexpr int   MAX_QUEUE_SIZE      =   3000;

DLL_API void    set_is_finish( bool flag );

DLL_API void    add_audio_frame( AVFrame* af );
DLL_API void    add_video_frame( AVFrame* vf );

DLL_API AVFrame*    get_audio_frame();
DLL_API AVFrame*    get_video_frame();

DLL_API bool    audio_need_wait();
DLL_API bool    video_need_wait();

DLL_API bool    is_audio_queue_full();
DLL_API bool    is_video_queue_full();

} // end namespace encode



// UI 跟 kernel 交換資料用的 functions.
namespace decode {

DLL_API void    add_audio_data( AudioData a_data );
DLL_API void    add_video_data( VideoData v_data );

DLL_API AudioData    get_audio_data();
DLL_API VideoData    get_video_data();

DLL_API void    clear_audio_queue();
DLL_API void    clear_video_queue();

DLL_API int     get_audio_size();
DLL_API int     get_video_size();

DLL_API bool&   get_v_seek_lock(); 
DLL_API bool&   get_a_seek_lock();

} // end namespace decode

#endif