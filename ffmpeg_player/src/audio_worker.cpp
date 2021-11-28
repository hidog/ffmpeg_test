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
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);
    



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

    //audio->setBufferSize( 1000000 );  // 遇到影片檔必須開大buffer不然會出問題. 研究一下怎麼取得這個參數....


    io      =   audio->start();


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
{}





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

    // start play
    audio_play();

    io->close();
    audio->stop();
    delete audio;
    audio = nullptr;

    MYLOG( LOG::INFO, "finish audio play." );
}




//extern std::mutex a_mtx;



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
    int buffer_size_half = 4096; //audio->bufferSize()/2;
    MYLOG( LOG::DEBUG, "buffer size half = %d", buffer_size_half );

    last   =   std::chrono::steady_clock::now();

    //
    while( is_play_end == false )
    {        
        if( a_queue->size() <= 0 )
        {
            MYLOG( LOG::WARN, "audio queue empty." );
            SLEEP_10MS;
            continue;
        }

        //
        a_mtx.lock();
        ad = a_queue->front();       
        a_queue->pop();
        a_mtx.unlock();

        //int rr = audio->bytesFree();
        //MYLOG( LOG::DEBUG, "bytesFree = %d\n", rr );

        // 某些影片檔,會解出超過buffer size的audio frame
        // 可以改變audio output的buffer size, 或是分批寫入

        while(true)
        {
            now         =   std::chrono::steady_clock::now();
            duration    =   std::chrono::duration_cast<std::chrono::milliseconds>( now - last );

            if( duration.count() >= ad.timestamp - last_ts )
                break;
        }

  
        int remain_bytes = ad.bytes;
        uint8_t *ptr = ad.pcm; 
        
        while(true)
        {
            //MYLOG( LOG::DEBUG, "bytesFree = %d\n", audio->bytesFree() );

            if( audio->bytesFree() < buffer_size_half )
                continue;
            else if( remain_bytes <= buffer_size_half )
            {
                int r = io->write( (const char*)ptr, remain_bytes );
                if( r != remain_bytes )
                    MYLOG( LOG::WARN, "r != remain_bytes" );
                delete [] ad.pcm;
                ad.pcm = nullptr;
                ad.bytes = 0;
                break;
            }
            else
            {
                int r = io->write( (const char*)ptr, buffer_size_half );
                if( r != buffer_size_half )
                    MYLOG( LOG::WARN, "r != buffer_size_half" );
                ptr += buffer_size_half;
                remain_bytes -= buffer_size_half;
            }
        }

        last_ts = ad.timestamp;
#if 0
        // old code
        //
        while( audio->bytesFree() < ad.bytes )
        {      
            //MYLOG( LOG::DEBUG, "bytesFree = %d\n", audio->bytesFree() );
        }   

        //
        int r = io->write( (const char*)ad.pcm, ad.bytes );
        if( r != ad.bytes )
            MYLOG( LOG::WARN, "r != ad.bytes" );

        //
        delete [] ad.pcm;
        ad.pcm      =   nullptr;
        ad.bytes    =   0;
#endif
    }

    // flush
    while( a_queue->empty() == false )
    {      
        //
        ad = a_queue->front();
        a_queue->pop();

        //
        while( audio->bytesFree() < ad.bytes )
        {      
            //printf("bytesFree = %d\n",r);
        }   

        //
        io->write( (const char*)ad.pcm, ad.bytes );

        //
        delete [] ad.pcm;
        ad.pcm      =   nullptr;
        ad.bytes    =   0;
    }
}
