#include "player_stream.h"
#include <thread>

extern "C" {

#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
#include <libavcodec/packet.h>

} // end extern "C"





/*******************************************************************************
PlayerStream::PlayerStream()
********************************************************************************/
PlayerStream::PlayerStream()
    :   Player()
{}



/*******************************************************************************
PlayerStream::~PlayerStream()
********************************************************************************/
PlayerStream::~PlayerStream()
{}




/*******************************************************************************
PlayerStream::init_live_stream()
********************************************************************************/
int    PlayerStream::init()
{
    // 必須先初始化
    Player::init();

    //
    AudioDecode &a_decoder  =   Player::get_audio_decoder();

    is_live_stream      =   true;
    AVSampleFormat  audio_fmt   =   static_cast<AVSampleFormat>(a_decoder.get_audio_sample_format());

    int   audio_channel     =   a_decoder.get_audio_channel();
    int   nb_sample         =   a_decoder.get_audio_nb_sample();
    int   bytes_per_sample  =   av_get_bytes_per_sample(audio_fmt);

    audio_pts_count     =   av_samples_get_buffer_size( NULL, audio_channel, nb_sample, audio_fmt, 0 );
    audio_pts_count     /=  audio_channel;
    audio_pts_count     /=  bytes_per_sample;

    //
    add_audio_frame_cb          =   std::bind( &encode::add_audio_frame, std::placeholders::_1 );
    add_video_frame_cb          =   std::bind( &encode::add_video_frame, std::placeholders::_1 );
    output_live_stream_func     =   std::bind( &PlayerStream::output_live_stream, this, std::placeholders::_1 );

    return  1;
}







/*******************************************************************************
PlayerStream::end_live_stream()
********************************************************************************/
int    PlayerStream::end()
{
    is_live_stream  =   false;
    Player::end();
    return  1;
}






/*******************************************************************************
PlayerStream::play_QT()

跟 Player::play_QT 相比, 移除了 seek 功能
********************************************************************************/
void    PlayerStream::play_QT()
{
    VideoDecode     &v_decoder  =   Player::get_video_decoder();
    AudioDecode     &a_decoder  =   Player::get_audio_decoder();
    SubDecode       &s_decoder  =   Player::get_subtitle_decoder();
    Demux           *demuxer    =   Player::get_demuxer();
    assert( demuxer != nullptr );

    // 目前先不處理字幕圖的live stream.
    if( is_live_stream == true && s_decoder.exist_stream() == true && s_decoder.is_graphic_subtitle() == true )
        MYLOG( LOG::ERROR, "un hanlde." )

    int         ret     =   0;
    AVPacket    *pkt    =   nullptr;
    Decode      *dc     =   nullptr;

    // 簡單的 sync 控制. 有空再修
    bool&   ui_v_seek_lock  =   decode::get_v_seek_lock();
    bool&   ui_a_seek_lock  =   decode::get_a_seek_lock();

    //
    while( stop_flag == false ) 
    {
        // NOTE: seek事件觸發的時候, queue 資料會暴增.
        while( demux_need_wait() == true )
        {
            //MYLOG( LOG::DEBUG, "v size = %d, a size = %d", video_queue.size(), audio_queue.size() );
            if( stop_flag == true )
                break;
            SLEEP_1MS;
        }

        //
        ret     =   demuxer->demux();
        if( ret < 0 )
            break;

        //
        pkt     =   demuxer->get_packet();
        if( v_decoder.find_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&v_decoder);
        else if( a_decoder.find_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else if( s_decoder.find_index( pkt->stream_index ) == true )
            dc  =   dynamic_cast<Decode*>(&s_decoder);  
        else
        {
            MYLOG( LOG::ERROR, "stream type not handle.");
            demuxer->unref_packet();
            continue;
        }

        //
        decode( dc, pkt );       
        demuxer->unref_packet();
    }
    MYLOG( LOG::INFO, "demux finish.");

    //
    flush();
    MYLOG( LOG::INFO, "play stream finish.")
}








/*******************************************************************************
PlayerStream::output_live_stream()

可以參考 
av_image_copy( v_frame->data, v_frame->linesize, (const uint8_t**)frame->data, frame->linesize, ctx->pix_fmt, ctx->width, ctx->height );
av_samples_copy( a_frame->data, frame->data, 0, 0, frame->nb_samples, frame->channels, fmt );
********************************************************************************/
void    PlayerStream::output_live_stream( Decode* dc )
{
    AVFrame*    frame   =   nullptr;
    AVFrame*    v_frame =   nullptr;
    AVFrame*    a_frame =   nullptr;

    if( dc->get_decode_context_type() == AVMEDIA_TYPE_VIDEO )
    {
        if( encode::is_video_queue_full() == true )
            return;

        v_frame     =   get_new_v_frame();
        frame       =   dc->get_frame();
        av_frame_copy( v_frame, frame );
        v_frame->pts =   dc->get_frame_count();        
        add_video_frame_cb(v_frame);
    }
    else if( dc->get_decode_context_type() == AVMEDIA_TYPE_AUDIO )
    {   
        if( encode::is_audio_queue_full() == true )
            return;

        a_frame     =   get_new_a_frame();
        frame       =   dc->get_frame();
        av_frame_copy( a_frame, frame );
        a_frame->pts    =   audio_pts_count * dc->get_frame_count();
        add_audio_frame_cb(a_frame);
    }
    else if( dc->get_decode_context_type() == AVMEDIA_TYPE_SUBTITLE )
    {
        // do nothing. 目前 live stream 不支援 subtitle stream.
    }
    else
    {
        MYLOG( LOG::ERROR, "un handle type" )
    }
}






/*******************************************************************************
PlayerStream::get_new_v_frame()
********************************************************************************/
AVFrame*    PlayerStream::get_new_v_frame()
{
    VideoDecode &v_decoder  =   Player::get_video_decoder();

    AVFrame*    v_frame     =   nullptr;
    int         ret         =   0;
    
    //
    v_frame   =   av_frame_alloc();
    if( v_frame == nullptr ) 
        MYLOG( LOG::ERROR, "v_frame alloc fail." );
    v_frame->pts  =   0;

    //
    v_frame->format   =   v_decoder.get_pix_fmt();
    v_frame->width    =   v_decoder.get_video_width();
    v_frame->height   =   v_decoder.get_video_height();

    ret     =   av_frame_get_buffer( v_frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "get buffer fail." );

    ret     =   av_frame_make_writable(v_frame);
    if( ret < 0 )
        assert(0);

    return  v_frame;
}







    
/*******************************************************************************
PlayerStream::get_new_a_frame()
********************************************************************************/
AVFrame*    PlayerStream::get_new_a_frame()
{
    AudioDecode &a_decoder  =   Player::get_audio_decoder();

    AVFrame*    a_frame     =   nullptr;
    int         ret         =   0;

    //
    a_frame   =   av_frame_alloc();
    if( a_frame == nullptr ) 
        MYLOG( LOG::ERROR, "a_frame alloc fail." );
    a_frame->pts  =   0;

    //
    a_frame->nb_samples       =     a_decoder.get_audio_nb_sample();
    a_frame->format           =     a_decoder.get_audio_sample_format();
    a_frame->channel_layout   =     a_decoder.get_audio_channel_layout();
    a_frame->channels         =     a_decoder.get_audio_channel();
    a_frame->sample_rate      =     a_decoder.get_audio_sample_rate();

    // 以 vorbis 來講,他會變動 sample 個數,暫時不支援這個壓縮格式
    if( a_frame->nb_samples == 0 )
        MYLOG( LOG::ERROR, "un support format." );

    //
    ret     =   av_frame_get_buffer( a_frame, 0 );
    if( ret < 0 ) 
        MYLOG( LOG::ERROR, "get buffer fail." );

    ret     =   av_frame_make_writable(a_frame);
    if( ret < 0 )
        assert(0);

    return  a_frame;
}



