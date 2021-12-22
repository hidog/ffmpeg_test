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
    v_setting.code_id   =   AV_CODEC_ID_MJPEG;
    v_setting.width     =   1280;
    v_setting.height    =   720;

    v_encoder.init(v_setting);
    a_encoder.init(AV_CODEC_ID_AAC);

    muxer.init( v_encoder.ctx, v_encoder.codec, a_encoder.ctx, a_encoder.codec );

}





/*******************************************************************************
Maker::work()
********************************************************************************/
void Maker::work()
{
    int ret;


    muxer.write_header();


    AVRational tb_a, tb_b;
    tb_a.num = 1001;
    tb_a.den = 24000;
    tb_b.num = 1;
    tb_b.den = 48000;

    AVFrame *v_frame = nullptr, *a_frame = nullptr;

    bool v_end = false, a_end = false;

    //while( encode_video == true || encode_audio == true ) 
    while(true)
    {
        /* select the stream to encode */
        auto ts_a = v_encoder.get_next_pts();
        auto ts_b = a_encoder.get_next_pts();

        ret = av_compare_ts( ts_a, tb_a, ts_b, tb_b );
        if( a_end == true )
            ret = 0;
        

        if( ret <= 0 && v_end == false )
        {
            v_frame = v_encoder.get_frame();
            if( v_frame == nullptr )
                v_end = true;

            ret = v_encoder.send_frame( v_frame );
            if( ret < 0 ) 
                MYLOG( LOG::ERROR, "send fail." );

            while( ret >= 0 ) 
            {
                ret = v_encoder.recv_frame();
                if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                    break;
                else if( ret < 0 )
                    MYLOG( LOG::ERROR, "recv fail." );

                auto pkt = v_encoder.get_pkt();

                //AVRational avr { 1, 24000 };  // 研究這邊怎麼來的,為什麼值會跑掉
                av_packet_rescale_ts( pkt, v_encoder.ctx->time_base, muxer.v_stream->time_base );

                muxer.write_frame( pkt );
            }  
        }
        else if( a_end == false )
        {
            a_frame = a_encoder.get_frame();
            MYLOG( LOG::DEBUG, "a fc = %d", a_encoder.frame_count );

            if( a_frame == nullptr )
                a_end = true;            

            ret = a_encoder.send_frame( a_frame );
            if( ret < 0 ) 
                MYLOG( LOG::ERROR, "send fail." );

            while( ret >= 0 ) 
            {
                ret = a_encoder.recv_frame();
                if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                    break;
                else if( ret < 0 )
                    MYLOG( LOG::ERROR, "recv fail." );

                auto pkt = a_encoder.get_pkt();
                muxer.write_frame( pkt );
            }  
        }      

        if( v_end == true && a_end == true )
            break;
    }


    //
    muxer.write_end();
}
