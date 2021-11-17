#include "worker.h"

#include <QDebug>



extern VideoData g_v_data;
extern std::mutex mtx;


Worker::Worker()
    :   QThread(nullptr)
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



void Worker::handleStateChanged( QAudio::State state )
{}




Worker::~Worker()
{}




void Worker::get_video_frame( QImage frame )
{
    emit recv_video_frame_signal();
}





void Worker::get_audio_pcm( AudioData ad )
{
#if 0
    g_ad = ad;
#else

    while( audio->bytesFree() < ad.bytes )
    {               
        int r = audio->bytesFree();
        printf("bytesFree = %d\n",r);
        std::this_thread::sleep_for( std::chrono::milliseconds(1) );
    }   
    
    io->write( (const char*)ad.pcm, ad.bytes );
    
    delete [] ad.pcm;
    ad.pcm = nullptr;
    ad.bytes = 0;
    
#endif
}



void Worker::run()  
{
    player.output_video_frame_func  =   std::bind( &Worker::get_video_frame, this, std::placeholders::_1 );
    player.output_audio_pcm_func    =   std::bind( &Worker::get_audio_pcm, this, std::placeholders::_1 );

    LOG

    thr_decode = new std::thread( &Worker::decode, this );
    thr_audio_play = new std::thread( &Worker::audio_play, this );
    thr_video_paly = new std::thread( &Worker::video_play, this );

    LOG

    thr_decode->join();
    thr_audio_play->join();
    thr_video_paly->join();

    delete thr_decode; thr_decode = nullptr;
    delete thr_audio_play; thr_audio_play = nullptr;
    delete thr_video_paly; thr_video_paly = nullptr;
}





void Worker::decode()
{
    player.init();
    player.play_QT();
    player.end();
}



void Worker::video_play()
{
    LOG

    std::queue<VideoData>* v_queue = get_video_queue();
    std::queue<AudioData>* a_queue = get_audio_queue();

    bool first_play = false;
    std::chrono::steady_clock::time_point start, now, last;

    while( v_queue->size() <= 3 )
    {
        LOG
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
    v_start = true;
    while( a_start == false )
    {
        // 這邊沒有print, 會造成optimize後無作用
        LOG  
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
    // 需要加上結束的時候跳出迴圈的功能.
    while(true)
    {       

        if( v_queue->size() <= 0 )
        {
            //std::this_thread::sleep_for( std::chrono::milliseconds(1) );
            continue;
        }

        VideoData vd = v_queue->front();
        v_queue->pop();

        if( first_play == false )
        {
            first_play = true;
            start = std::chrono::steady_clock::now();
            //last = std::chrono::steady_clock::now();
        }

        while(true)
        {
            now = std::chrono::steady_clock::now();
            std::chrono::duration<int64_t, std::milli> duration = std::chrono::duration_cast<std::chrono::milliseconds>( now - start );
            //std::chrono::duration<int, std::nano> duration = std::chrono::duration_cast<std::chrono::nanoseconds>( now - last );
            //if( duration.count() >= 161708333 )
            //if( duration.count() >= 41 708 333 ) // 需要修改程式,從frame的pts做控制
            auto cccc = duration.count();
            
            //printf( "%d    %lld\n", cccc, vd.ts );

            if( cccc >= vd.ts )
            //if( duration.count() >= 41708333 ) // 需要修改程式,從frame的pts做控制
            {
                //printf( "diff = %lf\n", duration.count() - vd.ts );
                //last = now;
                break;
            }

            //std::this_thread::sleep_for( std::chrono::milliseconds(1) );
        }
        /*000000000000
        1234567890AB
        1000000000000000000LL
        1000000*/

        mtx.lock();

        g_v_data.frame = vd.frame;
        g_v_data.index = vd.index;

        mtx.unlock();
            

        emit recv_video_frame_signal();
        //main_cb(&vd.frame);
    }
}




void Worker::audio_play()
{
#if 1
    LOG

    std::queue<VideoData>* v_queue = get_video_queue();
    std::queue<AudioData>* a_queue = get_audio_queue();

    while( a_queue->size() <= 3 )
    {
        LOG
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
    a_start = true;
    while( v_start == false )
    {
        LOG
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
        

    // 需要加上結束的時候跳出迴圈的功能.
    while(true)
    {
        
        if( a_queue->size() <= 0 )
        {
            //std::this_thread::sleep_for( std::chrono::milliseconds(1) );
            continue;
        }

        AudioData ad = a_queue->front();
        a_queue->pop();

        while( audio->bytesFree() < ad.bytes )
        {      
            //std::this_thread::sleep_for( std::chrono::milliseconds(1) );
            
            //int r = audio->bytesFree();
            //printf("bytesFree = %d\n",r);
            //std::this_thread::sleep_for( std::chrono::milliseconds(1) );
        }   

        io->write( (const char*)ad.pcm, ad.bytes );

        delete [] ad.pcm;
        ad.pcm = nullptr;
        ad.bytes = 0;
    }
#else
    while( true )
    {
        if( g_ad.pcm == nullptr )
            continue;

        while( audio->bytesFree() < g_ad.bytes )
        {
            player.pause();                    
            int r = audio->bytesFree();
            printf("bytesFree = %d\n",r);
        }

        player.resume();

        io->write( (const char*)g_ad.pcm, g_ad.bytes );

        delete [] g_ad.pcm;
        g_ad.pcm = nullptr;
        g_ad.bytes = 0;
    }
#endif
}
