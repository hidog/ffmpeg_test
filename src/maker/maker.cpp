#include "maker.h"
#include "tool.h"
#include "mux_io.h"

#include "audio_encode.h"
#include "video_encode.h"
#include "sub_encode.h"

#include <thread>
#include <mutex>


extern "C" {

#include <libavcodec/avcodec.h>

} // end extern "C"



/*******************************************************************************
Maker::Maker()
********************************************************************************/
Maker::Maker()
    :   MakerInterface()
{}




/*******************************************************************************
Maker::~Maker()
********************************************************************************/
Maker::~Maker()
{
    end();
}




/*******************************************************************************
Maker::init()
********************************************************************************/
void    Maker::init( EncodeSetting _setting, VideoEncodeSetting v_setting, AudioEncodeSetting a_setting, SubEncodeSetting s_setting )
{
    setting     =   _setting;

    //
    muxer   =   new Mux;
    muxer->init( setting );

    //
    bool    need_global_header  =   muxer->is_need_global_header();

    v_encoder.init( default_video_stream_index, v_setting, need_global_header );
    a_encoder.init( default_audio_stream_index, a_setting, need_global_header );
    if( setting.has_subtitle == true )
        s_encoder.init( default_subtitle_stream_index, s_setting, need_global_header );

    //
    auto v_ctx  =   v_encoder.get_ctx();
    auto a_ctx  =   a_encoder.get_ctx();
    auto s_ctx  =   s_encoder.get_ctx();

    muxer->open( setting, v_ctx, a_ctx, s_ctx );
}




/*******************************************************************************
Maker::is_connect()
********************************************************************************/
bool    Maker::is_connect()
{
    MYLOG( LOG::ERROR, "should not run into this code." );
    return  true;
}








/*******************************************************************************
Maker::flush_encoder()
********************************************************************************/
void    Maker::flush_encoder( Encode* enc )
{
    assert( enc != nullptr );

    int     ret     =   enc->flush();

    while( ret >= 0 ) 
    {
        ret     =   enc->recv_frame();
        if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
            break;
        else if( ret < 0 )
        {
            MYLOG( LOG::ERROR, "recv fail." );
            break;
        }
    
        enc->encode_timestamp();
        auto pkt        =   enc->get_pkt();
        muxer->write_frame( pkt );
        enc->unref_data();
    } 

    enc->set_flush(true);
}





/*******************************************************************************
Maker::work_with_subtitle()
********************************************************************************/
void    Maker::work_with_subtitle()
{
    int     ret     =   0;

    //
    s_encoder.load_all_subtitle();
    v_encoder.next_frame();
    a_encoder.next_frame();
    Encode*     encoder    =   nullptr;

    //
    while( v_encoder.end_of_file() == false || a_encoder.end_of_file() == false || s_encoder.end_of_file() == false )
    {       
        //
        if( v_encoder <= a_encoder )
        {
            if( v_encoder <= s_encoder )
                encoder     =   &v_encoder;
            else 
                encoder     =   &s_encoder;
        }
        else
        {
            if( a_encoder <= s_encoder )
                encoder     =   &a_encoder;
            else
                encoder     =   &s_encoder;
        }

        // send
        ret     =   encoder->send_frame();
        if( ret < 0 )
        {
            MYLOG( LOG::ERROR, "send fail." );
            break;
        }

        // recv
        while( ret >= 0 ) 
        {
            ret     =   encoder->recv_frame();
            if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                break;
            else if( ret < 0 )
            {
                MYLOG( LOG::ERROR, "recv fail." );
                break;
            }

            encoder->encode_timestamp();
            auto pkt    =   encoder->get_pkt();
            muxer->write_frame( pkt );
            encoder->unref_data();
        }  

        // update frame
        encoder->next_frame();

        /* 
            note: 理論上不用判斷 encoder->is_flush() == false, 
            因為 flush 後, 會處理另一個 stream, 所以無法跑進已經 flush 過的stream.
            保險起見加一個檢查
        */
        if( encoder->end_of_file() == true && encoder->is_flush() == false )
            flush_encoder( encoder );
    }

    //
    if( s_encoder.get_queue_size() > 0 )
        MYLOG( LOG::WARN, "subtitie queue is not empty." );
    if( s_encoder.is_flush() == false )
        MYLOG( LOG::WARN, "subtitie not flush." );
    
    // flush
    if( s_encoder.is_flush() == false )
        flush_encoder( &s_encoder );
    if( v_encoder.is_flush() == false )
        flush_encoder( &v_encoder );
    if( a_encoder.is_flush() == false )
        flush_encoder( &a_encoder );

    //
    muxer->write_end();
}









/*******************************************************************************
Maker::work_without_subtitle()
********************************************************************************/
void    Maker::work_without_subtitle()
{
    int     ret     =   0;
    Encode* encoder =   nullptr;

    v_encoder.next_frame(); 
    a_encoder.next_frame();    

    //
    while( v_encoder.end_of_file() == false || a_encoder.end_of_file() == false )
    {        
        // 用 pts 決定進去 mux 的順序
        if( v_encoder <= a_encoder )        
            encoder =   &v_encoder;        
        else        
            encoder =   &a_encoder;        

        // send
        ret     =   encoder->send_frame();
        if( ret < 0 )
        {
            MYLOG( LOG::ERROR, "send fail." );
            break;
        }

        // recv
        auto ctx_tb =   encoder->get_timebase();
        auto stb    =   encoder->get_stream_time_base();
        while( ret >= 0 ) 
        {
            ret     =   encoder->recv_frame();
            if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                break;
            else if( ret < 0 )
            {
                MYLOG( LOG::ERROR, "recv fail." );
                break;
            }

            encoder->encode_timestamp();
            auto pkt    =   encoder->get_pkt();
            //av_packet_rescale_ts( pkt, ctx_tb, stb );
            muxer->write_frame( pkt );
            encoder->unref_data();
        }  

        // update frame
        encoder->next_frame();

        /* 
            note: 理論上不用判斷 encoder->is_flush() == false, 
            因為 flush 後, 會處理另一個 stream, 所以無法跑進已經 flush 過的stream.
            保險起見加一個檢查
        */
        if( encoder->end_of_file() == true && encoder->is_flush() == false )
            flush_encoder( encoder );
    }
    
    // flush. 
    // note: 理論上已經在 loop 內 flush 完畢.
    if( v_encoder.is_flush() == false || a_encoder.is_flush() == false )
        MYLOG( LOG::WARN, "not flush in loop." );
    if( v_encoder.is_flush() == false )
        flush_encoder( &v_encoder );
    if( a_encoder.is_flush() == false )
        flush_encoder( &a_encoder );

    //
    muxer->write_end();
}






/*******************************************************************************
Maker::work()
********************************************************************************/
void Maker::work()
{
    muxer->write_header();

    // note: stream_time_base 會在 write header 後改變值, 所以需要再 write header 後做設置.
    AVRational  stb;
    stb     =   muxer->get_video_stream_timebase();
    v_encoder.set_stream_time_base(stb);
    stb     =   muxer->get_audio_stream_timebase();
    a_encoder.set_stream_time_base(stb);
    if( setting.has_subtitle == true )
    {
        stb     =   muxer->get_sub_stream_timebase();
        s_encoder.set_stream_time_base(stb);
    }

    if( setting.has_subtitle == true )
        work_with_subtitle();
    else
        work_without_subtitle();

    MYLOG( LOG::INFO, "encode finish." );
}




/*******************************************************************************
Maker::end()
********************************************************************************/
void Maker::end()
{
    v_encoder.end();
    a_encoder.end();
    s_encoder.end();

    if( muxer != nullptr )
    {
        muxer->end();
        delete muxer;
        muxer   =   nullptr;
    }
}






