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
AudioWorker::open_audio_output()
********************************************************************************/
void    AudioWorker::open_audio_output( AudioSetting as )
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
        MYLOG( LOG::ERROR, "Raw audio format not supported by backend, cannot play audio." );
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


    int     volume  =   dynamic_cast<MainWindow*>(parent())->volume();
    volume_slot(volume);

    if( io != nullptr )
        MYLOG( LOG::ERROR, "io is not null." );

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
    MYLOG( LOG::DEBUG, "state = %d", state );
}





/*******************************************************************************
AudioWorker::~AudioWorker()
********************************************************************************/
AudioWorker::~AudioWorker()
{}




/*******************************************************************************
AudioWorker::audio_play()
********************************************************************************/
void AudioWorker::run()  
{
    if( io == nullptr )
    {
        MYLOG( LOG::ERROR, "io is null." );
        return;
    }
    
    if( audio == nullptr )
    {
        MYLOG( LOG::ERROR, "audio is null." );
        return;
    }

    force_stop  =   false;

    // start play
    audio_play();

    io->close();
    io  =   nullptr;

    audio->stop();
    delete audio;
    audio   =   nullptr;

    MYLOG( LOG::INFO, "finish audio play." );
}





/*******************************************************************************
AudioWorker::audio_play()
********************************************************************************/
void AudioWorker::audio_play()
{
    MYLOG( LOG::INFO, "start play audio" );

    std::mutex& a_mtx = get_a_mtx();
    
    AudioData   ad;
    std::queue<AudioData>*  a_queue     =   get_audio_queue();
    bool    &v_start        =   dynamic_cast<MainWindow*>(parent())->get_video_worker()->get_video_start_state();
    bool    &is_play_end    =   dynamic_cast<MainWindow*>(parent())->get_worker()->get_play_end_state();

    while( a_queue->size() <= 3 )
        SLEEP_10MS;
    a_start = true;
    while( v_start == false )
        SLEEP_10MS;        

    std::chrono::steady_clock::time_point       last, now;
    std::chrono::duration<int64_t, std::milli>  duration;
    int64_t last_ts = 0;
    
    // 一次寫入4096的效果比較好. 用原本的作法會有機會破音. 記得修改變數名稱
    int     wanted_buffer_size  =   4096; //audio->bufferSize()/2;

    //
    int     remain          =   0;
    int     remain_bytes    =   0;
    uint8_t *ptr            =   nullptr; 

    //
    auto    handle_func    =   [&]() 
    {
        //
        a_mtx.lock();
        ad  =   a_queue->front();       
        a_queue->pop();
        a_mtx.unlock();

        while(true)
        {
            now         =   std::chrono::steady_clock::now();
            duration    =   std::chrono::duration_cast<std::chrono::milliseconds>( now - last );

            if( duration.count() >= ad.timestamp - last_ts )
                break;
        }

        remain_bytes    =   ad.bytes;
        ptr             =   ad.pcm; 

        while(true)
        {
            //MYLOG( LOG::DEBUG, "bytesFree = %d\n", audio->bytesFree() );

            if( audio->bytesFree() < wanted_buffer_size )
                continue;
            else if( remain_bytes <= wanted_buffer_size )
            {
                remain  =   io->write( (const char*)ptr, remain_bytes );
                if( remain != remain_bytes )
                    MYLOG( LOG::WARN, "remain != remain_bytes" );
                delete [] ad.pcm;
                ad.pcm      =   nullptr;
                ad.bytes    =   0;
                break;
            }
            else
            {
                remain  =   io->write( (const char*)ptr, wanted_buffer_size );
                if( remain != wanted_buffer_size )
                    MYLOG( LOG::WARN, "r != wanted_buffer_size" );
                ptr             +=  wanted_buffer_size;
                remain_bytes    -=  wanted_buffer_size;
            }
        }

        last_ts = ad.timestamp;
    };

    //
    last   =   std::chrono::steady_clock::now();
    while( is_play_end == false && force_stop == false )
    {        
        if( a_queue->size() <= 0 )
        {
            MYLOG( LOG::WARN, "audio queue empty." );
            SLEEP_10MS;
            continue;
        }

        handle_func();
    }

    // flush
    while( a_queue->empty() == false && force_stop == false )
    {      
        if( a_queue->size() <= 0 )
        {
            MYLOG( LOG::WARN, "audio queue empty." );
            SLEEP_10MS;
            continue;
        }

        handle_func();
    }

    // 等 player 結束, 確保不會再增加資料進去queue
    while( is_play_end == false )
        SLEEP_10MS;

    // force stop 需要手動清除 queue.
    while( a_queue->empty() == false )
    {
        ad  =   a_queue->front();
        delete [] ad.pcm;
        a_queue->pop();
    }
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
            MYLOG( LOG::ERROR, "not handle");
    }
}
