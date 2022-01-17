#include "maker_io.h"








/*******************************************************************************
Maker::release_encode_video_frame()
********************************************************************************/
void    MakerIO::release_encode_video_frame( AVFrame *vf )
{
#if 0
    av_frame_free( &vf );
#endif
}




/*******************************************************************************
Maker::release_encode_audio_frame()
********************************************************************************/
void    MakerIO::release_encode_audio_frame( AVFrame *af )
{
#if 0
    av_frame_free( &af );
#endif
}





/*******************************************************************************
io_write_data
********************************************************************************/
int     io_write_data( void *opaque, uint8_t *buf, int buf_size )
{
    return  1;

#if 0
    InputOutput*    io  =   (InputOutput*)opaque;
    int     ret     =   io->write( buf, buf_size );
    return  ret;
#endif
}







/*******************************************************************************
MakerIO::work_live_stream()
********************************************************************************/
void    MakerIO::work_live_stream()
{
#if 0
    // wait for data to start.
    while( aframe_queue.size() < 10 || vframe_queue.size() < 10 )    
    //while( aframe_queue.size() < 10 )
        SLEEP_10MS;    

    //
    int     ret;

    muxer->write_header();

    AVRational v_time_base  =   v_encoder->get_timebase();
    AVRational a_time_base  =   a_encoder->get_timebase();

    //
    AVFrame *v_frame    =   get_video_frame();
    v_encoder->set_frame( v_frame );
    //AVFrame *v_frame    =   v_encoder->get_frame();


    AVFrame *a_frame    =   get_audio_frame();  
    a_encoder->set_frame( a_frame );
    //AVFrame *a_frame = a_encoder->get_frame();

    //AVFrame *frame      =   nullptr;
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
            muxer->write_frame( pkt );
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
            if( v_encoder->is_flush() == false )
            {
                st_tb   =   muxer->get_video_stream_timebase();
                flush_func( v_encoder );
            }
            result     =   1;
        }
        else if( a_frame == nullptr )
        {
            if( a_encoder->is_flush() == false )
            {
                st_tb   =   muxer->get_audio_stream_timebase();
                flush_func( a_encoder );  
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
            encoder =   v_encoder;
            //frame   =   v_frame;
            st_tb   =   muxer->get_video_stream_timebase();
        }
        else if( a_frame != nullptr ) // audio
        {
            encoder =   a_encoder;
            //frame   =   a_frame;
            st_tb   =   muxer->get_audio_stream_timebase();
        }
        else
        {
            MYLOG( LOG::WARN, "both not");
            break;
        }

        //assert( frame != nullptr );

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

            MuxIO*  mux_io  =   dynamic_cast<MuxIO*>(muxer);
            if( mux_io == nullptr )            
                MYLOG( LOG::ERROR, "mux_io is null." );

            while( mux_io->io_need_wait() == true )          
                SLEEP_1MS;

            muxer->write_frame( pkt );
            encoder->unref_pkt();
        }  

        // update frame
        if( encoder == v_encoder ) // �z�Q�O�� enum �B�z, �o������i, ���ŭ�
        {
            release_encode_video_frame( v_frame );
            v_frame     =   get_video_frame();
            encoder->set_frame( v_frame );
            //v_frame = v_encoder->get_frame();
        }
        else
        {
            release_encode_audio_frame( a_frame );
            a_frame     =   get_audio_frame();
            encoder->set_frame( a_frame );
            //a_frame = a_encoder->get_frame();
        }
    }
    
    // flush
    if( v_encoder->is_flush() == false )
    {
        st_tb   =   muxer->get_video_stream_timebase();
        flush_func( v_encoder );
    }

    if( a_encoder->is_flush() == false )
    {
        st_tb   =   muxer->get_audio_stream_timebase();
        flush_func( a_encoder );
    }

    //
    muxer->write_end();
#endif
}
