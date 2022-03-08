#include "audio_worker.h"

#include <QDebug>

#include "video_worker.h"
#include "worker.h"
#include "mainwindow.h"





/*******************************************************************************
AudioWorker::AudioWorker()
********************************************************************************/
AudioWorker::AudioWorker( QObject *parent )
    :   QThread(parent)
{}




/*******************************************************************************
AudioWorker::~AudioWorker()
********************************************************************************/
AudioWorker::~AudioWorker()
{}





/*******************************************************************************
AudioWorker::set_only_audio()
********************************************************************************/
void    AudioWorker::set_only_audio( bool flag )
{
    only_audio  =   flag;
}




/*******************************************************************************
AudioWorker::open_audio_output()
********************************************************************************/
void    AudioWorker::open_audio_output( AudioDecodeSetting as )
{
    QAudioFormat    format;

    // Set up the format, eg.
    format.setSampleRate(as.sample_rate);
    //format.setChannelCount(as.channel);
    format.setChannelCount(2);  // 目前強制兩聲道,未來改成可以多聲道或單聲道
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    //format.setSampleType(QAudioFormat::UnSignedInt);
    format.setSampleType(QAudioFormat::SignedInt);   // 用unsigned int 在調整音量的時候會爆音
    
    //
    QAudioDeviceInfo info { QAudioDeviceInfo::defaultOutputDevice() };
    if( false == info.isFormatSupported(format) ) 
    {
        MYLOG( LOG::L_ERROR, "Raw audio format not supported by backend, cannot play audio." );
        return;
    }

    //
    if( audio != nullptr )
    {
        io->close();
        audio->stop();
        delete audio;
    }

    //
    audio   =   new QAudioOutput( info, format );
    connect( audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)) );
    audio->stop();
    //audio->setBufferSize( 1000000 );  // 遇到影片檔必須開大buffer不然會出問題. 這是一個解法,目前用分批寫入的方式解決

    MainWindow  *mw     =   dynamic_cast<MainWindow*>(parent());
    if( mw == nullptr )
        MYLOG( LOG::L_ERROR, "mw is nullptr");

    int     volume      =   mw->volume();
    volume_slot(volume);

    if( io != nullptr )
        MYLOG( LOG::L_ERROR, "io is not null." );

    io  =   audio->start();
}





/*******************************************************************************
AudioWorker::get_audio_start_state()
********************************************************************************/
bool&   AudioWorker::get_audio_start_state()
{
    return  a_start;
}





/*******************************************************************************
AudioWorker::handleStateChanged()
********************************************************************************/
void    AudioWorker::handleStateChanged( QAudio::State state )
{
    MYLOG( LOG::L_DEBUG, "state = %d", state );
}






/*******************************************************************************
AudioWorker::audio_play()
********************************************************************************/
void AudioWorker::run()  
{
    if( io == nullptr )
    {
        MYLOG( LOG::L_ERROR, "io is null." );
        return;
    }
    
    if( audio == nullptr )
    {
        MYLOG( LOG::L_ERROR, "audio is null." );
        return;
    }

    force_stop  =   false;
    seek_flag   =   false;

    // start play
    if( only_audio == true )
        audio_play();
    else
        audio_play_with_video();

    io->close();
    io  =   nullptr;

    audio->stop();
    delete audio;
    audio   =   nullptr;

    MYLOG( LOG::L_INFO, "finish audio play." );
}








/*******************************************************************************
AudioWorker::seek_slot()
********************************************************************************/
void    AudioWorker::seek_slot( int sec )
{
    seek_flag   =   true;
    a_start     =   false;
}





/*******************************************************************************
AudioWorker::flush_for_seek()

seek 的時候, 由 UI 端負責清空資料, 之後等到有資料才繼續播放.
由 player 清空的話, 會遇到 queue 是否為空的判斷不好寫, 
沒寫好會造成 handle_func crash.
********************************************************************************/
void    AudioWorker::flush_for_seek()
{
    if( only_audio == true )
    {
        while( decode::get_audio_size() <= 3 )
            SLEEP_10MS;
        a_start     =   true;
    }
    else
    {
        bool    &v_start    =   dynamic_cast<MainWindow*>(parent())->get_video_worker()->get_video_start_state();

        // 重新等待有資料才播放
        while( decode::get_audio_size() <= 3 )
            SLEEP_10MS;
        a_start     =   true;
        while( v_start == false )
            SLEEP_10MS;
    }
}





/*******************************************************************************
AudioWorker::audio_play()
********************************************************************************/
void    AudioWorker::audio_play()
{
    MYLOG( LOG::L_INFO, "start play audio" );

    AudioData   ad;
    bool    &is_play_end    =   dynamic_cast<MainWindow*>(parent())->get_worker()->get_play_end_state();

    while( decode::get_audio_size() <= 30 )
        SLEEP_10MS; 

    std::chrono::steady_clock::time_point       last, now;
    std::chrono::duration<int64_t, std::milli>  duration;
    int64_t last_ts =   0;
    
    // 一次寫入4096的效能比較好
    int     wanted_buffer_size  =   4096; //audio->bufferSize()/2;
    int     remain          =   0;
    int     remain_bytes    =   0;
    uint8_t *ptr            =   nullptr; 

    // 練習使用 lambda operator.
    auto    handle_func    =   [&]() 
    {
        //
        ad  =   decode::get_audio_data();

        remain_bytes    =   ad.bytes;
        ptr             =   ad.pcm; 

        while(true)
        {
            //MYLOG( LOG::L_DEBUG, "bytesFree = %d\n", audio->bytesFree() );

            if( audio->bytesFree() < wanted_buffer_size )
            {
                SLEEP_1MS;
                continue;
            }
            else if( remain_bytes <= wanted_buffer_size )
            {
                remain  =   io->write( (const char*)ptr, remain_bytes );
                if( remain != remain_bytes )
                    MYLOG( LOG::L_WARN, "remain != remain_bytes" );
                delete [] ad.pcm;
                ad.pcm      =   nullptr;
                ad.bytes    =   0;
                break;
            }
            else
            {
                remain  =   io->write( (const char*)ptr, wanted_buffer_size );
                if( remain != wanted_buffer_size )
                    MYLOG( LOG::L_WARN, "r != wanted_buffer_size" );
                ptr             +=  wanted_buffer_size;
                remain_bytes    -=  wanted_buffer_size;
            }

            if( seek_flag == false )
                emit    update_seekbar_signal( ad.timestamp / 1000 );
        }

        last_ts = ad.timestamp;
    };

    // 用 bool 簡單做 sync, 有空再修
    bool    &ui_a_seek_lock     =   decode::get_a_seek_lock();

    last   =   std::chrono::steady_clock::now();
    while( is_play_end == false && force_stop == false )
    {        
        if( seek_flag == true )
        {
            seek_flag   =   false;
            last        =   std::chrono::steady_clock::time_point();
            last_ts     =   INT64_MAX;
            ui_a_seek_lock  =   true;
            while( ui_a_seek_lock == true )
                SLEEP_10MS;
            flush_for_seek();
        }

        if( decode::get_audio_size() <= 0 )
        {
            MYLOG( LOG::L_WARN, "audio queue empty." );
            SLEEP_10MS;
            continue;
        }

        handle_func();
    }

    // flush
    while( decode::get_audio_size() > 0 && force_stop == false )
    {      
        if( decode::get_audio_size() <= 0 )
        {
            MYLOG( LOG::L_WARN, "audio queue empty." );
            SLEEP_10MS;
            continue;
        }

        handle_func();
    }

    // 等 player 結束, 確保不會再增加資料進去queue
    while( is_play_end == false )
        SLEEP_10MS;

    // force stop 需要手動清除 queue.
    decode::clear_audio_queue();
}






/*******************************************************************************
AudioWorker::audio_play()
********************************************************************************/
void AudioWorker::audio_play_with_video()
{
    MYLOG( LOG::L_INFO, "start play audio" );

    AudioData   ad;
    bool    &v_start        =   dynamic_cast<MainWindow*>(parent())->get_video_worker()->get_video_start_state();
    bool    &is_play_end    =   dynamic_cast<MainWindow*>(parent())->get_worker()->get_play_end_state();

    while( decode::get_audio_size() <= 30 )
        SLEEP_10MS;
    a_start = true;
    while( v_start == false )
        SLEEP_10MS;        

    std::chrono::steady_clock::time_point       last, now;
    std::chrono::duration<int64_t, std::milli>  duration;
    int64_t last_ts =   0;
    
    // 一次寫入4096的效能比較好
    int     wanted_buffer_size  =   4096; //audio->bufferSize()/2;

    //
    int     remain          =   0;
    int     remain_bytes    =   0;
    uint8_t *ptr            =   nullptr; 

    // 練習使用 lambda operator.
    auto    handle_func    =   [&]() 
    {
        //
        ad  =   decode::get_audio_data();

        while(true)
        {
            now         =   std::chrono::steady_clock::now();
            duration    =   std::chrono::duration_cast<std::chrono::milliseconds>( now - last );

            if( duration.count() >= ad.timestamp - last_ts )
                break;
        }
        
#if 0
        // 測試用的 a/v sync 程式, live stream 的時候比較容易發生不同步.
        // 這邊是容許值範圍外會讓影音同步.
        // 這邊有點醜, 需要把函數搬出去.
        static volatile VideoData    *view_data  =   dynamic_cast<MainWindow*>(parent())->get_view_data();
        assert( view_data != nullptr );
        if( ad.timestamp < view_data->timestamp - 100 )
        {
            MYLOG( LOG::L_WARN, "trigger a/v sync. skip audio" );
            // skip this audio data.
            do {
                delete [] ad.pcm;
                ad.pcm      =   nullptr;
                ad.bytes    =   0;
                ad  =   decode::get_audio_data();
            } while( ad.timestamp <= view_data->timestamp + 100 );
        }
        else if( ad.timestamp > view_data->timestamp + 100 )
        {
            MYLOG( LOG::L_WARN, "trigger a/v sync. wait video. ad = %lld, vd = %lld", ad.timestamp, view_data->timestamp );
            while( ad.timestamp >= view_data->timestamp - 100 )
                SLEEP_1MS;
        }
#endif

        remain_bytes    =   ad.bytes;
        ptr             =   ad.pcm; 

        while(true)
        {
            //MYLOG( LOG::L_DEBUG, "bytesFree = %d\n", audio->bytesFree() );

            if( audio->bytesFree() < wanted_buffer_size )
                continue;
            else if( remain_bytes <= wanted_buffer_size )
            {
                remain  =   io->write( (const char*)ptr, remain_bytes );
                if( remain != remain_bytes )
                    MYLOG( LOG::L_WARN, "remain != remain_bytes" );
                delete [] ad.pcm;
                ad.pcm      =   nullptr;
                ad.bytes    =   0;
                break;
            }
            else
            {
                remain  =   io->write( (const char*)ptr, wanted_buffer_size );
                if( remain != wanted_buffer_size )
                    MYLOG( LOG::L_WARN, "r != wanted_buffer_size" );
                ptr             +=  wanted_buffer_size;
                remain_bytes    -=  wanted_buffer_size;
            }
        }

        last_ts = ad.timestamp;
    };

    // 用 bool 簡單做 sync, 有空再修
    bool    &ui_a_seek_lock     =   decode::get_a_seek_lock();

    last   =   std::chrono::steady_clock::now();
    while( is_play_end == false && force_stop == false )
    {        
        if( seek_flag == true )
        {
            seek_flag   =   false;
            last        =   std::chrono::steady_clock::time_point();
            last_ts     =   INT64_MAX;
            ui_a_seek_lock  =   true;
            while( ui_a_seek_lock == true )
                SLEEP_10MS;
            flush_for_seek();
        }

        if( decode::get_audio_size() <= 0 )
        {
            MYLOG( LOG::L_WARN, "audio queue empty." );
            SLEEP_10MS;
            continue;
        }

        handle_func();
    }

    // flush
    while( decode::get_audio_size() > 0 && force_stop == false )
    {      
        if( decode::get_audio_size() <= 0 )
        {
            MYLOG( LOG::L_WARN, "audio queue empty." );
            SLEEP_10MS;
            continue;
        }

        handle_func();
    }

    // 等 player 結束, 確保不會再增加資料進去queue
    while( is_play_end == false )
        SLEEP_10MS;

    // force stop 需要手動清除 queue.
    decode::clear_audio_queue();
}






/*******************************************************************************
AudioWorker::stop()
********************************************************************************/
void    AudioWorker::stop()
{
    force_stop  =   true;
}






/*******************************************************************************
AudioWorker::volume_slot()
********************************************************************************/
void    AudioWorker::volume_slot( int value )
{
    if( audio != nullptr )
    {
        // 直接設置數字會出問題, 參考官方寫法做轉換
        qreal   linear_value    =   QAudio::convertVolume( value / qreal(100.0),
                                                           QAudio::LogarithmicVolumeScale,
                                                           QAudio::LinearVolumeScale );

        audio->setVolume(linear_value);
    }
}




/*******************************************************************************
AudioWorker::get_volume()
********************************************************************************/
int     AudioWorker::get_volume()
{
    if( audio != nullptr )
    {
        qreal   rv      =   audio->volume();
        int     value   =   qRound( rv * 100 );
        return  value;
    }
}





/*******************************************************************************
AudioWorker::pause()
********************************************************************************/
void    AudioWorker::pause()
{
    if( audio != nullptr )
    {
        QAudio::State    st  =   audio->state();
        if( st == QAudio::SuspendedState )
            audio->resume();
        else if( st == QAudio::ActiveState )
            audio->suspend();
        else
            MYLOG( LOG::L_ERROR, "not handle");
    }
}
