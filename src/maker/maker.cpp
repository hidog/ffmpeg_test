#include "maker.h"
#include "tool.h"


extern "C" {

//#include <libavcodec/codec_id.h>
//#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>

}





/*******************************************************************************
Maker::Maker()
********************************************************************************/
Maker::Maker()
{}




/*******************************************************************************
Maker::~Maker()
********************************************************************************/
Maker::~Maker()
{}






/*******************************************************************************
Maker::init()
********************************************************************************/
void Maker::init( VideoEncodeSetting v_setting, AudioEncodeSetting a_setting )
{
    v_encoder.init( 0, v_setting );
    a_encoder.init( 1, a_setting );

    auto v_ctx      =   v_encoder.get_ctx();
    auto v_codec    =   v_encoder.get_codec();
    auto a_ctx      =   a_encoder.get_ctx();
    auto a_codec    =   a_encoder.get_codec();

    muxer.init( v_ctx, v_codec, a_ctx, a_codec );
}





/*******************************************************************************
Maker::work()
********************************************************************************/
void Maker::work()
{
    int ret;

    muxer.write_header();

    AVRational v_time_base  =   v_encoder.get_timebase();
    AVRational a_time_base  =   a_encoder.get_timebase();

    //
    AVFrame *v_frame    =   v_encoder.get_frame(); 
    AVFrame *a_frame    =   a_encoder.get_frame();    
    AVFrame *frame      =   nullptr;
    Encode  *encoder    =   nullptr;

    AVRational st_tb;

    // flush function.
    auto flush_func = [&] ( Encode *enc ) 
    {
        ret     =   enc->flush();  
        while( ret >= 0 ) 
        {
            ret     =   enc->recv_frame();
            if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                break;
            else if( ret < 0 )
                MYLOG( LOG::ERROR, "recv fail." );

            auto pkt        =   enc->get_pkt();
            auto ctx_tb     =   enc->get_timebase();
            av_packet_rescale_ts( pkt, ctx_tb, st_tb );
            muxer.write_frame( pkt );
        } 
        enc->set_flush(true);
    };

    // determine stream pts function
    auto order_pts_func = [&] () -> int
    {
        int     result  =   0;
        int64_t v_pts, a_pts;

        // �z�פW���ΦҼ{��ӳ��O nullptr �� case,�b loop ����N�ư��o��Ʊ��F.
        if( v_frame == nullptr ) 
        {
            // flush video.
            // �����b�o��B�z,��loop���A�B�z�����|�y��stream flush�������g�J�ɶ����~.  (�Ҧp�n����v���u)
            if( v_encoder.is_flush() == false )
            {
                st_tb   =   muxer.get_video_stream_timebase();
                flush_func( &v_encoder );
            }
            result     =   1;
        }
        else if( a_frame == nullptr )
        {
            if( a_encoder.is_flush() == false )
            {
                st_tb   =   muxer.get_audio_stream_timebase();
                flush_func( &a_encoder );  
            }
            result     =   -1;
        }
        else
        {
            // �쥻�Q�� INT64_MAX, ���|�y�� overflow.
            v_pts   =   v_frame->pts;
            a_pts   =   a_frame->pts;
            result  =   av_compare_ts( v_pts, v_time_base, a_pts, a_time_base );
        }

        return  result;
    };


    // �𮧤@�U�A�ӫ�ҳo�����g, �Ʊ�g�o�n�ݤ@�I
    while( v_frame != nullptr || a_frame != nullptr )
    {        
        assert( v_frame != nullptr || a_frame != nullptr );

        //
        ret     =   order_pts_func();

        //
        if( ret <= 0 && v_frame != nullptr ) // video
        {
            encoder =   &v_encoder;
            frame   =   v_frame;
            st_tb   =   muxer.get_video_stream_timebase();
        }
        else if( a_frame != nullptr ) // audio
        {
            encoder =   &a_encoder;
            frame   =   a_frame;
            st_tb   =   muxer.get_audio_stream_timebase();
        }
        else
        {
            MYLOG( LOG::WARN, "both not");
            break;
        }

        assert( frame != nullptr );

        //
        ret     =   encoder->send_frame();
        if( ret < 0 ) 
            MYLOG( LOG::ERROR, "send fail." );

        while( ret >= 0 ) 
        {
            ret     =   encoder->recv_frame();
            if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                break;
            else if( ret < 0 )
                MYLOG( LOG::ERROR, "recv fail." );

            auto pkt    =   encoder->get_pkt();
            auto ctx_tb =   encoder->get_timebase();
            av_packet_rescale_ts( pkt, ctx_tb, st_tb );

            muxer.write_frame( pkt );
        }  

        // update frame
        if( encoder == &v_encoder ) // �z�Q�O�� enum �B�z, �o������i, ���ŭ�
            v_frame     =   encoder->get_frame();
        else
            a_frame     =   encoder->get_frame();
    }
    
    // flush
    if( v_encoder.is_flush() == false )
    {
        st_tb   =   muxer.get_video_stream_timebase();
        flush_func( &v_encoder );
    }

    if( a_encoder.is_flush() == false )
    {
        st_tb   =   muxer.get_audio_stream_timebase();
        flush_func( &a_encoder );
    }

    //
    muxer.write_end();
}




/*******************************************************************************
Maker::end()
********************************************************************************/
void Maker::end()
{
    v_encoder.end();
    a_encoder.end();
    muxer.end();
}
