#include "audio_worker.h"

#include <QDebug>









/*******************************************************************************
AudioWorker::AudioWorker()
********************************************************************************/
AudioWorker::AudioWorker( QObject *parent )
    :   QThread(parent)
{}




/*******************************************************************************
AudioWorker::open_audio_output()
********************************************************************************/
void    AudioWorker::open_audio_output()
{
    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);



    QAudioDeviceInfo info { QAudioDeviceInfo::defaultOutputDevice() };
    if( false == info.isFormatSupported(format) ) 
    {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        return;
    }

    audio = new QAudioOutput( info, format );
    connect( audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)) );
    audio->stop();

    io = audio->start(); // 未來要從 mainwindow 做驅動


                         //thr = new std::thread( &Worker::audio_play, this );
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

    MYLOG( LOG::INFO, "finish audio play." );
}








/*******************************************************************************
AudioWorker::audio_play()
********************************************************************************/
void AudioWorker::audio_play()
{
    MYLOG( LOG::INFO, "start play audio" );

    std::queue<AudioData>*  a_queue     =   get_audio_queue();
    AudioData   ad;

    while( a_queue->size() <= 3 )
        SLEEP_10MS;
    a_start = true;
    while( v_start == false )
        SLEEP_10MS;        

    // 需要加上結束的時候跳出迴圈的功能.
    while(true)
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

}
