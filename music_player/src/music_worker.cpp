#include "music_worker.h"

#include <QDebug>


#include "play_worker.h"
#include "ui/mainwindow.h"





/*******************************************************************************
MusicWorker::MusicWorker()
********************************************************************************/
MusicWorker::MusicWorker( QObject *parent )
    :   QThread(parent)
{}




/*******************************************************************************
MusicWorker::~MusicWorker()
********************************************************************************/
MusicWorker::~MusicWorker()
{}






/*******************************************************************************
MusicWorker::open_audio_output()
********************************************************************************/
void    MusicWorker::open_audio_output( AudioDecodeSetting as )
{
    QAudioFormat    format;

    // Set up the format, eg.
    format.setSampleRate(as.sample_rate);
    //format.setChannelCount(as.channel);
    format.setChannelCount(2);  // 目前強制兩聲道,未來改成可以多聲道或單聲道
    //format.setSampleSize(16);
    format.setSampleSize(as.sample_size);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    //format.setSampleType(QAudioFormat::UnSignedInt);
    //format.setSampleType(QAudioFormat::SignedInt);   // 用unsigned int 在調整音量的時候會爆音
    format.setSampleType( static_cast<QAudioFormat::SampleType>(as.sample_type) );

    //
    QAudioDeviceInfo info { QAudioDeviceInfo::defaultOutputDevice() };
    if( false == info.isFormatSupported(format) ) 
    {
        MYLOG( LOG::L_ERROR, "Raw audio format not supported by backend, cannot play audio." );
        return;
    }

    //
    /*if( audio != nullptr )
    {
        io->close();
        audio->stop();
        delete audio;
    }*/

    //
    if( audio == nullptr )
    {
        audio   =   new QAudioOutput( info, format );
        connect( audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)) );
    }
    audio->stop();
    //audio->setBufferSize( 1000000 );  // 遇到影片檔必須開大buffer不然會出問題. 這是一個解法,目前用分批寫入的方式解決

    MainWindow  *main_window     =   dynamic_cast<MainWindow*>(parent());
    if( main_window == nullptr )
        MYLOG( LOG::L_ERROR, "mw is nullptr");

    int     volume      =   main_window->volume();
    volume_slot(volume);

    //if( io != nullptr )
      //  MYLOG( LOG::L_ERROR, "io is not null." );

    io  =   audio->start();
}





/*******************************************************************************
MusicWorker::get_audio_start_state()
********************************************************************************/
bool&   MusicWorker::get_audio_start_state()
{
    return  a_start;
}






/*******************************************************************************
MusicWorker::handleStateChanged()
********************************************************************************/
void    MusicWorker::handleStateChanged( QAudio::State state )
{
    MYLOG( LOG::L_DEBUG, "state = %d", state );
}






/*******************************************************************************
MusicWorker::audio_play()
********************************************************************************/
void    MusicWorker::run()  
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

    timestamp   =   0;
    current_sec =   0;
    force_stop  =   false;
    seek_flag   =   false;

    // start play
    audio_play();

#if 0
    io->close();
    io  =   nullptr;

    audio->stop();
    delete audio;
    audio   =   nullptr;
#endif

    current_sec =   0;

    MYLOG( LOG::L_INFO, "finish audio play." );
}





/*******************************************************************************
MusicWorker::seek_slot()
********************************************************************************/
void    MusicWorker::close_io()
{
    if( io != nullptr )
    {
        io->close();
        io  =   nullptr;
    }

    if( audio != nullptr )
    {
        audio->stop();
        delete audio;
        audio   =   nullptr;
    }
}






/*******************************************************************************
MusicWorker::seek_slot()
********************************************************************************/
void    MusicWorker::seek_slot( int sec )
{
    seek_flag   =   true;
    a_start     =   false;
}





/*******************************************************************************
MusicWorker::flush_for_seek()
********************************************************************************/
void    MusicWorker::flush_for_seek()
{
    while( decode::get_audio_size() <= 10 )
        SLEEP_10MS;
    a_start     =   true;
}






/*******************************************************************************
MusicWorker::audio_play()
********************************************************************************/
void    MusicWorker::audio_play()
{
    MYLOG( LOG::L_INFO, "start play audio" );

    bool    &is_play_end    =   dynamic_cast<MainWindow*>(parent())->get_play_worker()->get_play_end_state();

    while( decode::get_audio_size() <= 10 )
        SLEEP_10MS;
    a_start = true;    

    std::chrono::steady_clock::time_point       last, now;
    std::chrono::duration<int64_t, std::milli>  duration;
    int64_t last_ts =   0;
    
    // 一次寫入4096的效能比較好
    int     wanted_buffer_size  =   4096; //audio->bufferSize()/2;

    //
    int     remain          =   0;
    int     remain_bytes    =   0;
    uint8_t *ptr            =   nullptr; 

    AudioData   ad;

    // 練習使用 lambda operator.
    auto    handle_func    =   [&]() 
    {
        //
        ad  =   decode::get_audio_data();

        /*while(true)
        {
            now         =   std::chrono::steady_clock::now();
            duration    =   std::chrono::duration_cast<std::chrono::milliseconds>( now - last );
            if( duration.count() >= ad.timestamp - last_ts )
                break;
        }*/

        remain_bytes    =   ad.bytes;
        ptr             =   ad.pcm; 

        while(true)
        {
            //MYLOG( LOG::L_DEBUG, "bytesFree = %d\n", audio->bytesFree() );

            if( audio->bytesFree() < wanted_buffer_size )
            {
                SLEEP_1US;
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
        }

        if( seek_flag == false )
        {
            emit update_seekbar_signal( ad.timestamp/1000 );
            timestamp   =   ad.timestamp;
        }

        last_ts =   ad.timestamp;
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

    MYLOG( LOG::L_INFO, "play audio finish." );

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
MusicWorker::stop()
********************************************************************************/
const int&  MusicWorker::get_current_sec()
{
    return  current_sec;
}





/*******************************************************************************
MusicWorker::stop()
********************************************************************************/
void    MusicWorker::stop()
{
    force_stop  =   true;
}






/*******************************************************************************
MusicWorker::volume_slot()
********************************************************************************/
void    MusicWorker::volume_slot( int value )
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
MusicWorker::get_volume()
********************************************************************************/
int     MusicWorker::get_volume()
{
    if( audio != nullptr )
    {
        qreal   rv      =   audio->volume();
        int     value   =   qRound( rv * 100 );
        return  value;
    }
}





/*******************************************************************************
MusicWorker::pause()
********************************************************************************/
void    MusicWorker::pause()
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





/*******************************************************************************
MusicWorker::is_pause()
********************************************************************************/
bool    MusicWorker::is_pause()
{
    if( audio != nullptr )
    {
        QAudio::State    st  =   audio->state();
        if( st == QAudio::SuspendedState )
            return  true;
        else
            return  false;
    }
    else
        return  false;
}






/*******************************************************************************
MusicWorker::get_timestamp()
********************************************************************************/
int64_t     MusicWorker::get_timestamp()
{
    return  timestamp;
}
