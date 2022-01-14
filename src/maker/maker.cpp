#include "maker.h"
#include "tool.h"
#include "mux_io.h"


extern "C" {

#include <libavcodec/avcodec.h>

} // end extern "C"





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
Maker::init_muxer()
********************************************************************************/
void    Maker::init_muxer()
{
    if( setting.io_type == IO_Type::DEFAULT )    
        muxer   =   new Mux;    
    else     
        muxer   =   new MuxIO;
    muxer->init( setting );
}




/*******************************************************************************
Maker::init()
********************************************************************************/
void Maker::init( EncodeSetting _setting, VideoEncodeSetting v_setting, AudioEncodeSetting a_setting, SubEncodeSetting s_setting )
{
    setting     =   _setting;

    init_muxer();

    bool    need_global_header  =   muxer->is_need_global_header();

    v_encoder.init( 0, v_setting, need_global_header );
    a_encoder.init( 1, a_setting, need_global_header );
    if( setting.has_subtitle == true )
        s_encoder.init( 2, s_setting, need_global_header );

    //
    auto v_ctx  =   v_encoder.get_ctx();
    auto a_ctx  =   a_encoder.get_ctx();
    auto s_ctx  =   s_encoder.get_ctx();

    muxer->open( setting, v_ctx, a_ctx, s_ctx );
}







/*******************************************************************************
Maker::work_with_subtitle()
********************************************************************************/
void    Maker::work_with_subtitle()
{
    int     ret     =   0;

    s_encoder.load_all_subtitle();
    muxer->write_header();

    AVRational  v_time_base     =   v_encoder.get_timebase();
    AVRational  a_time_base     =   a_encoder.get_timebase();
    AVRational  s_time_base     =   s_encoder.get_src_stream_timebase();

    //
    AVFrame*    v_frame    =   v_encoder.get_frame(); 
    AVFrame*    a_frame    =   a_encoder.get_frame();    
    AVFrame*    frame      =   nullptr;
    Encode*     encoder    =   nullptr;

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
    // EncodeOrder, �]���ثe�u�B�z�@�� v/a/s, �ҥH�ΦC�|
    // �n�B�z�h�� a/s, �N�����^�� stream index ����.
    auto order_pts_func = [&, this] () -> EncodeOrder
    {
        int         ret     =   0;
        int64_t     v_pts, a_pts, s_pts;
        EncodeOrder order;

        // �z�פW���ΦҼ{��ӳ��O nullptr �� case,�b loop ����N�ư��o��Ʊ��F.
        if( v_frame == nullptr ) 
        {
            // flush video.
            // �����b�o��B�z,�� loop ���A�B�z�����|�y�� stream flush �������g�J�ɶ����~.  
            // �Ҧp�n����v���u, �ɭP flush ���q�� audio frame pts �b video frame �e��, ���g�J�o�b�᭱.
            if( v_encoder.is_flush() == false )
            {
                st_tb   =   muxer->get_video_stream_timebase();
                flush_func( &v_encoder );
            }
        }
        else if( a_frame == nullptr )
        {
            if( a_encoder.is_flush() == false )
            {
                st_tb   =   muxer->get_audio_stream_timebase();
                flush_func( &a_encoder );  
            }
        }

        // note: �p�G�ݭn�䴩�h audio/subtitle, �o��ݭn���s�]�p
        if( v_frame == nullptr && a_frame == nullptr )
            order   =   EncodeOrder::SUBTITLE;
        else if( v_frame == nullptr && s_encoder.get_queue_size() == 0 )
            order   =   EncodeOrder::AUDIO;
        else if( a_frame == nullptr && s_encoder.get_queue_size() == 0 )
            order   =   EncodeOrder::VIDEO;
        else if( s_encoder.get_queue_size() == 0 )
        {
            v_pts   =   v_frame->pts;
            a_pts   =   a_frame->pts;
            ret     =   av_compare_ts( v_pts, v_time_base, a_pts, a_time_base );
            if( ret <= 0 )
                order   =   EncodeOrder::VIDEO;
            else
                order   =   EncodeOrder::AUDIO;
        }
        else if( v_frame == nullptr )
        {
            a_pts   =   a_frame->pts;
            s_pts   =   s_encoder.get_pts();
            ret     =   av_compare_ts( a_pts, a_time_base, s_pts, s_time_base );
            if( ret <= 0 )
                order   =   EncodeOrder::AUDIO;
            else
                order   =   EncodeOrder::SUBTITLE;
        }
        else if( a_frame == nullptr )
        {
            v_pts   =   v_frame->pts;
            s_pts   =   s_encoder.get_pts();
            ret     =   av_compare_ts( v_pts, v_time_base, s_pts, s_time_base );
            if( ret <= 0 )
                order   =   EncodeOrder::VIDEO;
            else
                order   =   EncodeOrder::SUBTITLE;
        }
        else
        {
            // �쥻�Q�� INT64_MAX, ���|�y�� overflow.
            v_pts   =   v_frame->pts;
            a_pts   =   a_frame->pts;
            s_pts   =   s_encoder.get_pts();
            ret     =   av_compare_ts( v_pts, v_time_base, a_pts, a_time_base );
            if( ret <= 0 )
            {
                ret     =   av_compare_ts( v_pts, v_time_base, s_pts, s_time_base );
                if( ret <= 0 )
                    order   =   EncodeOrder::VIDEO;
                else
                    order   =   EncodeOrder::SUBTITLE;
            }
            else
            {
                ret     =   av_compare_ts( a_pts, a_time_base, s_pts, s_time_base );
                if( ret <= 0 )
                    order   =   EncodeOrder::AUDIO;
                else
                    order   =   EncodeOrder::SUBTITLE;
            }
        }

        return  order;
    };

    // �𮧤@�U�A�ӫ�ҳo�����g, �Ʊ�g�o�n�ݤ@�I
    EncodeOrder     order;
    AVPacket*       pkt =   nullptr;
    AVRational      ctx_tb;
    while( v_frame != nullptr || a_frame != nullptr || s_encoder.get_queue_size() > 0 )
    {        
        assert( v_frame != nullptr || a_frame != nullptr || s_encoder.get_queue_size() > 0 );

        //
        order   =   order_pts_func();

        //
        if( order == EncodeOrder::VIDEO ) // video
        {
            encoder =   &v_encoder;
            frame   =   v_frame;
            st_tb   =   muxer->get_video_stream_timebase();
        }
        else if( order == EncodeOrder::AUDIO ) // audio
        {
            encoder =   &a_encoder;
            frame   =   a_frame;
            st_tb   =   muxer->get_audio_stream_timebase();
        }
        else if( order == EncodeOrder::SUBTITLE ) // subtitle        
            st_tb   =   muxer->get_sub_stream_timebase();
        else
            assert(0);

        //
        if( order == EncodeOrder::SUBTITLE )
        {
            s_encoder.encode_subtitle();

            pkt     =   s_encoder.get_pkt();
            ctx_tb  =   s_encoder.get_timebase();
            int64_t     subtitle_pts    =   s_encoder.get_subtitle_pts();
            int64_t     duration        =   s_encoder.get_duration();

            pkt->pts         =   av_rescale_q( subtitle_pts, AVRational{1,AV_TIME_BASE}, st_tb );
            pkt->duration    =   av_rescale_q( duration, ctx_tb, st_tb );
            pkt->dts         =   pkt->pts;
            s_encoder.set_last_pts( pkt->pts );

            muxer->write_frame( pkt );
            // ����ݬݨ쩳�ݤ��ݭn unref.
            s_encoder.unref_subtitle();
            //s_encoder.unref_pkt();
        }
        else
        {
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

                pkt    =   encoder->get_pkt();
                ctx_tb =   encoder->get_timebase();
                av_packet_rescale_ts( pkt, ctx_tb, st_tb );

                muxer->write_frame( pkt );
                encoder->unref_pkt();
            }  
            // update frame
            if( encoder == &v_encoder ) // �z�Q�O�� enum �B�z, �o������i, ���ŭ�
                v_frame     =   encoder->get_frame();
            else
                a_frame     =   encoder->get_frame();
        }
    }

    //
    if( s_encoder.get_queue_size() > 0 )
        MYLOG( LOG::ERROR, "subtitie queue is not empty." );

    // flush subtitle    
    s_encoder.generate_flush_pkt();
    pkt     =   s_encoder.get_pkt();
    muxer->write_frame( pkt );
    
    // flush
    if( v_encoder.is_flush() == false )
    {
        st_tb   =   muxer->get_video_stream_timebase();
        flush_func( &v_encoder );
    }

    if( a_encoder.is_flush() == false )
    {
        st_tb   =   muxer->get_audio_stream_timebase();
        flush_func( &a_encoder );
    }

    //
    muxer->write_end();
}



/*******************************************************************************
Maker::work_without_subtitle()
********************************************************************************/
void    Maker::work_without_subtitle()
{
    int ret;

    muxer->write_header();

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
            if( v_encoder.is_flush() == false )
            {
                st_tb   =   muxer->get_video_stream_timebase();
                flush_func( &v_encoder );
            }
            result     =   1;
        }
        else if( a_frame == nullptr )
        {
            if( a_encoder.is_flush() == false )
            {
                st_tb   =   muxer->get_audio_stream_timebase();
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
            st_tb   =   muxer->get_video_stream_timebase();
        }
        else if( a_frame != nullptr ) // audio
        {
            encoder =   &a_encoder;
            frame   =   a_frame;
            st_tb   =   muxer->get_audio_stream_timebase();
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

            muxer->write_frame( pkt );
            encoder->unref_pkt();
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
        st_tb   =   muxer->get_video_stream_timebase();
        flush_func( &v_encoder );
    }

    if( a_encoder.is_flush() == false )
    {
        st_tb   =   muxer->get_audio_stream_timebase();
        flush_func( &a_encoder );
    }

    //
    muxer->write_end();
}






/*******************************************************************************
Maker::work()
********************************************************************************/
void Maker::work()
{
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

    muxer->end();
    delete muxer;
    muxer   =   nullptr;
}





/*******************************************************************************
maker_encode_example
********************************************************************************/
void    maker_encode_example()
{
    EncodeSetting   setting;
    //setting.io_type =   IO_Type::DEFAULT;
    //setting.io_type =   IO_Type::FILE_IO;
    setting.io_type =   IO_Type::SRT_IO;


    // rmvb �O variable bitrate. �ثe�ٵL�k�ϥ�
    //setting.filename    =   "H:\\output.mkv";
    //setting.extension   =   "matroska";
    //setting.filename    =   "H:\\output.mp4";
    //setting.extension   =   "mp4";
    //setting.filename    =   "H:\\test.avi"; 
    //setting.extension   =   "avi";
    setting.srt_port    =   "1234";

    setting.has_subtitle    =   false;



    VideoEncodeSetting  v_setting;
    v_setting.load_jpg_root_path    =   "J:\\jpg";
    //v_setting.code_id   =   AV_CODEC_ID_H264;
    v_setting.code_id   =   AV_CODEC_ID_H265;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG1VIDEO;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG2VIDEO;

    v_setting.width     =   1280;
    v_setting.height    =   720;

    v_setting.time_base.num     =   1001;
    v_setting.time_base.den     =   24000;

    /*
        b frame not support on rm
    */
    //v_setting.gop_size      =   30;
    //v_setting.max_b_frames  =   15; 
    v_setting.gop_size      =   10;
    v_setting.max_b_frames  =   0;


    v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P12LE;

    v_setting.src_width     =   1280;
    v_setting.src_height    =   720;
    //v_setting.src_pix_fmt   =   AV_PIX_FMT_BGRA;    // for QImage
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGR24;    // for openCV


    AudioEncodeSetting  a_setting;
    a_setting.load_pcm_path     =   "J:\\test.pcm";
    //a_setting.code_id     =   AV_CODEC_ID_MP3;
    a_setting.code_id       =   AV_CODEC_ID_AAC;
    //a_setting.code_id       =   AV_CODEC_ID_AC3;
    //a_setting.code_id     =   AV_CODEC_ID_MP2;
    //a_setting.code_id       =   AV_CODEC_ID_VORBIS;
    //a_setting.code_id       =   AV_CODEC_ID_FLAC;

    //a_setting.bit_rate          =   320000;
    a_setting.bit_rate          =   128000;
    a_setting.sample_rate       =   48000;
    a_setting.channel_layout    =   3; // AV_CH_LAYOUT_STEREO = 3;




    SubEncodeSetting   s_setting;
    s_setting.code_id       =   AV_CODEC_ID_ASS;
    //s_setting.code_id       =   AV_CODEC_ID_SUBRIP;
    //s_setting.code_id       =   AV_CODEC_ID_MOV_TEXT;
    s_setting.subtitle_file =   "J:\\test.ssa";
    s_setting.subtitle_ext  =   "ssa";


    Maker   maker;

    maker.init( setting, v_setting, a_setting, s_setting );
    maker.work();
    maker.end();
}






/*******************************************************************************
io_write_data
********************************************************************************/
int     io_write_data( void *opaque, uint8_t *buf, int buf_size )
{
    InputOutput*    io  =   (InputOutput*)opaque;
    int     ret     =   io->write( buf, buf_size );
    return  ret;
}
