#ifndef TOOL_H
#define TOOL_H

#include <cassert>
#include <QImage>




struct AudioData
{
    uint8_t     *pcm;
    int         bytes;
};


struct AudioSetting
{
    int     channel;
    int     sample_rate;
    //int     sample_size;
};



struct VideoData
{
    int         index;
    QImage      frame;
    int64_t     timestamp;
};



struct VideoSetting
{
    int     width;
    int     height;
}




constexpr int ERROR = -1;
constexpr int SUCCESS = 0;
constexpr int HAVE_FRAME = 1; // 需要用 1 來判斷是否還有 frame.



       
#define SLEEP_10MS std::this_thread::sleep_for( std::chrono::milliseconds(10) );





enum class LOG
{
    DEBUG = 0,
    INFO,
    WARN,
    ERROR
};



#define MYLOG( TAG, ... ) \
	{ \
        if( TAG == LOG::DEBUG ) \
		    printf("[DEBUG] [%s] [%d] ", __FILE__, __LINE__); \
        else if( TAG == LOG::INFO ) \
            printf("[INFO] [%s] [%d] ", __FILE__, __LINE__); \
        else if( TAG == LOG::WARN ) \
            printf("[WARN] [%s] [%d] ", __FILE__, __LINE__); \
        else \
            printf("[ERR] [%s] [%d] ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
        if( TAG == LOG::ERROR ) \
            assert(0); \
	}




#ifdef FFMPEG_TEST
#define DLL_API
#elif defined(_BUILD_DLL)
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif


#endif