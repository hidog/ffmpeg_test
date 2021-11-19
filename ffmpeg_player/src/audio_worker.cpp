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
    format.setChannelCount(as.channel);
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
        audio->stop();
        io->close();
        delete audio;
    }

    //
    audio   =   new QAudioOutput( info, format );
    connect( audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)) );
    audio->stop();

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








/*******************************************************************************
AudioWorker::audio_play()
********************************************************************************/
void AudioWorker::audio_play()
{
    MYLOG( LOG::INFO, "start play audio" );
    
    AudioData   ad;
    std::queue<AudioData>*  a_queue     =   get_audio_queue();
    bool    &v_start        =   dynamic_cast<MainWindow*>(parent())->get_video_worker()->get_video_start_state();
    bool    &is_play_end    =   dynamic_cast<MainWindow*>(parent())->get_worker()->get_play_end_state();

    while( a_queue->size() <= 3 )
        SLEEP_10MS;
    a_start = true;
    while( v_start == false )
        SLEEP_10MS;        

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
