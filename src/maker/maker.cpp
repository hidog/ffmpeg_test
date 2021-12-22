#include "maker.h"
#include "tool.h"


extern "C" {

#include <libavcodec/codec_id.h>
#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

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
void Maker::init()
{
    VideoEncodeSetting v_setting;
    v_setting.code_id   =   AV_CODEC_ID_H264;
    v_setting.width     =   1280;
    v_setting.height    =   720;

    AudioEncodeSetting a_setting;
    a_setting.code_id = AV_CODEC_ID_AAC;
    a_setting.bit_rate = 320000;
    a_setting.sample_rate = 48000;

    v_encoder.init( 0, v_setting);
    a_encoder.init( 1, a_setting);

    muxer.init( v_encoder.get_ctx(), v_encoder.get_codec(), a_encoder.get_ctx(), a_encoder.get_codec() );

}





/*******************************************************************************
Maker::work()
********************************************************************************/
void Maker::work()
{
    int ret;

    muxer.write_header();

    /*AVRational tb_a, tb_b;
    tb_a.num = 1001;
    tb_a.den = 24000;
    tb_b.num = 1;
    tb_b.den = 48000;*/

    AVRational v_time_base  =   v_encoder.get_timebase();
    AVRational a_time_base  =   a_encoder.get_timebase();

    //
    AVFrame *v_frame    =   v_encoder.get_frame(); 
    AVFrame *a_frame    =   a_encoder.get_frame();    
    AVFrame *frame      =   nullptr;
    Encode  *encoder    =   nullptr;

    AVRational st_tb;

    int64_t v_pts, a_pts;

    // 休息一下再來思考這邊怎麼改寫, 希望寫得好看一點
    while( v_frame != nullptr || a_frame != nullptr )
    {        
        // 原本想用 INT64_MAX, 但會造成 overflow.
        v_pts = v_frame == nullptr ? -1 : v_frame->pts;
        a_pts = a_frame == nullptr ? -1 : a_frame->pts;

        ret = av_compare_ts( v_pts, v_time_base, a_pts, a_time_base );

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

            auto ctx_tb = encoder->get_timebase();
            av_packet_rescale_ts( pkt, ctx_tb, st_tb );

            muxer.write_frame( pkt );
        }  

        // update frame
        if( encoder == &v_encoder ) // 理想是用 enum 處理, 這邊先偷懶, 有空修
            v_frame = encoder->get_frame();
        else
            a_frame = encoder->get_frame();
    }

    // flush
    // 未來思考這邊如何寫得更好.
    auto flush_func = [&] ( Encode *enc ) 
    {
        ret =   enc->send_frame();  
        while( ret >= 0 ) 
        {
            ret     =   enc->recv_frame();
            if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                break;
            else if( ret < 0 )
                MYLOG( LOG::ERROR, "recv fail." );

            auto pkt    =   enc->get_pkt();
            auto ctx_tb =   enc->get_timebase();
            av_packet_rescale_ts( pkt, ctx_tb, st_tb );
            muxer.write_frame( pkt );
        } 
    };
    
    st_tb = muxer.get_video_stream_timebase();
    flush_func( &v_encoder );

    st_tb = muxer.get_audio_stream_timebase();
    flush_func( &a_encoder );  

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
