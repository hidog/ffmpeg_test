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
{
    a_encoder   =   new AudioEncode;
    v_encoder   =   new VideoEncode;
    s_encoder   =   new SubEncode;    

    setting     =   new EncodeSetting;
}




/*******************************************************************************
Maker::~Maker()
********************************************************************************/
Maker::~Maker()
{
    delete a_encoder;
    a_encoder    =   nullptr;

    delete v_encoder;
    v_encoder    =   nullptr;

    delete s_encoder;
    s_encoder    =   nullptr;

    delete setting;
    setting     =   nullptr;
}



/*******************************************************************************
Maker::init_muxer()
********************************************************************************/
void    Maker::init_muxer()
{
    if( setting->io_type == IO_Type::DEFAULT )    
        muxer   =   new Mux;    
    else     
        muxer   =   new MuxIO;
    muxer->init( *setting );
}




/*******************************************************************************
Maker::init()
********************************************************************************/
void    Maker::init( EncodeSetting* _setting, VideoEncodeSetting* v_setting, AudioEncodeSetting* a_setting, SubEncodeSetting* s_setting )
{
    *setting     =   *_setting;

    init_muxer();

    bool    need_global_header  =   muxer->is_need_global_header();

    v_encoder->init( 0, *v_setting, need_global_header );
    a_encoder->init( 1, *a_setting, need_global_header );
    if( setting->has_subtitle == true )
        s_encoder->init( 2, *s_setting, need_global_header );

    //
    auto v_ctx  =   v_encoder->get_ctx();
    auto a_ctx  =   a_encoder->get_ctx();
    auto s_ctx  =   s_encoder->get_ctx();

    muxer->open( *setting, v_ctx, a_ctx, s_ctx );
}




/*******************************************************************************
Maker::is_connect()
********************************************************************************/
bool    Maker::is_connect()
{
    if( muxer == nullptr ) // not init yet.
        return  false;

    bool    flag    =   muxer->is_connect();
    return  flag;
}








/*******************************************************************************
Maker::order_pts_func()

 determine stream pts function
 EncodeOrder, 因為目前只處理一組 v/a/s, 所以用列舉
 要處理多組 a/s, 就必須回傳 stream index 之類.
********************************************************************************/
//EncodeOrder Maker::order_pts_func()
void Maker::order_pts_func()
{
#if 0
    int         ret     =   0;
    int64_t     v_pts, a_pts, s_pts;
    EncodeOrder order;

    // 理論上不用考慮兩個都是 nullptr 的 case,在 loop 控制就排除這件事情了.
    if( v_frame == nullptr ) 
    {
        // flush video.
        // 必須在這邊處理,等 loop 完再處理有機會造成 stream flush 的部分寫入時間錯誤.  
        // 例如聲音比影像短, 導致 flush 階段的 audio frame pts 在 video frame 前面, 但寫入卻在後面.
        if( v_encoder->is_flush() == false )
        {
            st_tb   =   muxer->get_video_stream_timebase();
            flush_func( v_encoder );
        }
    }
    else if( a_frame == nullptr )
    {
        if( a_encoder->is_flush() == false )
        {
            st_tb   =   muxer->get_audio_stream_timebase();
            flush_func( a_encoder );  
        }
    }

    // note: 如果需要支援多 audio/subtitle, 這邊需要重新設計
    if( v_frame == nullptr && a_frame == nullptr )
        order   =   EncodeOrder::SUBTITLE;
    else if( v_frame == nullptr && s_encoder->get_queue_size() == 0 )
        order   =   EncodeOrder::AUDIO;
    else if( a_frame == nullptr && s_encoder->get_queue_size() == 0 )
        order   =   EncodeOrder::VIDEO;
    else if( s_encoder->get_queue_size() == 0 )
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
        s_pts   =   s_encoder->get_pts();
        ret     =   av_compare_ts( a_pts, a_time_base, s_pts, s_time_base );
        if( ret <= 0 )
            order   =   EncodeOrder::AUDIO;
        else
            order   =   EncodeOrder::SUBTITLE;
    }
    else if( a_frame == nullptr )
    {
        v_pts   =   v_frame->pts;
        s_pts   =   s_encoder->get_pts();
        ret     =   av_compare_ts( v_pts, v_time_base, s_pts, s_time_base );
        if( ret <= 0 )
            order   =   EncodeOrder::VIDEO;
        else
            order   =   EncodeOrder::SUBTITLE;
    }
    else
    {
        // 原本想用 INT64_MAX, 但會造成 overflow.
        v_pts   =   v_frame->pts;
        a_pts   =   a_frame->pts;
        s_pts   =   s_encoder->get_pts();
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
#endif
}







/*******************************************************************************
Maker::flush_encoder()
********************************************************************************/
void    Maker::flush_encoder( Encode* enc, AVRational* st_tb )
{
    assert( enc != nullptr && st_tb != nullptr );

    int ret     =   enc->flush();  
    while( ret >= 0 ) 
    {
        ret     =   enc->recv_frame();
        if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
            break;
        else if( ret < 0 )
            MYLOG( LOG::ERROR, "recv fail." );
    
        auto pkt        =   enc->get_pkt();
        auto ctx_tb     =   enc->get_timebase();
        av_packet_rescale_ts( pkt, ctx_tb, *st_tb );
        muxer->write_frame( pkt );
    } 
    enc->set_flush(true);
}





/*******************************************************************************
Maker::work_with_subtitle()
********************************************************************************/
void    Maker::work_with_subtitle()
{
    int     ret     =   0;

    s_encoder->load_all_subtitle();
    muxer->write_header();

    AVRational  v_time_base     =   v_encoder->get_timebase();
    AVRational  a_time_base     =   a_encoder->get_timebase();
    AVRational  s_time_base     =   s_encoder->get_src_stream_timebase();

    //
    AVFrame*    v_frame    =   v_encoder->get_frame(); 
    AVFrame*    a_frame    =   a_encoder->get_frame();    
    AVFrame*    frame      =   nullptr;
    Encode*     encoder    =   nullptr;

    AVRational st_tb;

    // flush function.
#if 0
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
#endif

    // determine stream pts function
    // EncodeOrder, 因為目前只處理一組 v/a/s, 所以用列舉
    // 要處理多組 a/s, 就必須回傳 stream index 之類.
#if 0
    auto order_pts_func = [&, this] () -> EncodeOrder
    {
        int         ret     =   0;
        int64_t     v_pts, a_pts, s_pts;
        EncodeOrder order;

        // 理論上不用考慮兩個都是 nullptr 的 case,在 loop 控制就排除這件事情了.
        if( v_frame == nullptr ) 
        {
            // flush video.
            // 必須在這邊處理,等 loop 完再處理有機會造成 stream flush 的部分寫入時間錯誤.  
            // 例如聲音比影像短, 導致 flush 階段的 audio frame pts 在 video frame 前面, 但寫入卻在後面.
            if( v_encoder->is_flush() == false )
            {
                st_tb   =   muxer->get_video_stream_timebase();
                flush_func( v_encoder );
            }
        }
        else if( a_frame == nullptr )
        {
            if( a_encoder->is_flush() == false )
            {
                st_tb   =   muxer->get_audio_stream_timebase();
                flush_func( a_encoder );  
            }
        }

        // note: 如果需要支援多 audio/subtitle, 這邊需要重新設計
        if( v_frame == nullptr && a_frame == nullptr )
            order   =   EncodeOrder::SUBTITLE;
        else if( v_frame == nullptr && s_encoder->get_queue_size() == 0 )
            order   =   EncodeOrder::AUDIO;
        else if( a_frame == nullptr && s_encoder->get_queue_size() == 0 )
            order   =   EncodeOrder::VIDEO;
        else if( s_encoder->get_queue_size() == 0 )
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
            s_pts   =   s_encoder->get_pts();
            ret     =   av_compare_ts( a_pts, a_time_base, s_pts, s_time_base );
            if( ret <= 0 )
                order   =   EncodeOrder::AUDIO;
            else
                order   =   EncodeOrder::SUBTITLE;
        }
        else if( a_frame == nullptr )
        {
            v_pts   =   v_frame->pts;
            s_pts   =   s_encoder->get_pts();
            ret     =   av_compare_ts( v_pts, v_time_base, s_pts, s_time_base );
            if( ret <= 0 )
                order   =   EncodeOrder::VIDEO;
            else
                order   =   EncodeOrder::SUBTITLE;
        }
        else
        {
            // 原本想用 INT64_MAX, 但會造成 overflow.
            v_pts   =   v_frame->pts;
            a_pts   =   a_frame->pts;
            s_pts   =   s_encoder->get_pts();
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
#endif

    // 休息一下再來思考這邊怎麼改寫, 希望寫得好看一點
    EncodeOrder     order;
    AVPacket*       pkt =   nullptr;
    AVRational      ctx_tb;
    while( v_frame != nullptr || a_frame != nullptr || s_encoder->get_queue_size() > 0 )
    {        
        assert( v_frame != nullptr || a_frame != nullptr || s_encoder->get_queue_size() > 0 );

        //
        //order   =   order_pts_func();
        if( *v_encoder <= *a_encoder )
        {
            if( *v_encoder <= *s_encoder )
                order   =   EncodeOrder::VIDEO;
            else 
                order   =   EncodeOrder::SUBTITLE;
        }
        else
        {
            if( *a_encoder <= *s_encoder )
                order   =   EncodeOrder::AUDIO;
            else
                order   =   EncodeOrder::SUBTITLE;
        }

        //
        if( order == EncodeOrder::VIDEO ) // video
        {
            encoder =   v_encoder;
            frame   =   v_frame;
            st_tb   =   muxer->get_video_stream_timebase();
        }
        else if( order == EncodeOrder::AUDIO ) // audio
        {
            encoder =   a_encoder;
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
            s_encoder->encode_subtitle();

            pkt     =   s_encoder->get_pkt();
            ctx_tb  =   s_encoder->get_timebase();
            int64_t     subtitle_pts    =   s_encoder->get_subtitle_pts();
            int64_t     duration        =   s_encoder->get_duration();

            pkt->pts         =   av_rescale_q( subtitle_pts, AVRational{1,AV_TIME_BASE}, st_tb );
            pkt->duration    =   av_rescale_q( duration, ctx_tb, st_tb );
            pkt->dts         =   pkt->pts;
            s_encoder->set_last_pts( pkt->pts );

            muxer->write_frame( pkt );
            // 實驗看看到底需不需要 unref.
            s_encoder->unref_subtitle();
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
            if( encoder == v_encoder ) // 理想是用 enum 處理, 這邊先偷懶, 有空修
                v_frame     =   encoder->get_frame();
            else
                a_frame     =   encoder->get_frame();

            if( v_frame == nullptr && v_encoder->is_flush() == false )
            {
                st_tb   =   muxer->get_video_stream_timebase();
                flush_encoder( v_encoder, &st_tb );
            }
            if( a_frame == nullptr && a_encoder->is_flush() == false )
            {
                st_tb   =   muxer->get_audio_stream_timebase();
                flush_encoder( a_encoder, &st_tb );
            }
        }
    }

    //
    if( s_encoder->get_queue_size() > 0 )
        MYLOG( LOG::ERROR, "subtitie queue is not empty." );

    // flush subtitle    
    s_encoder->generate_flush_pkt();
    pkt     =   s_encoder->get_pkt();
    muxer->write_frame( pkt );
    
    // flush
    if( v_encoder->is_flush() == false )
    {
        st_tb   =   muxer->get_video_stream_timebase();
        flush_encoder( v_encoder, &st_tb );
    }

    if( a_encoder->is_flush() == false )
    {
        st_tb   =   muxer->get_audio_stream_timebase();
        flush_encoder( a_encoder, &st_tb );
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

    AVRational v_time_base  =   v_encoder->get_timebase();
    AVRational a_time_base  =   a_encoder->get_timebase();

    //
    AVFrame *v_frame    =   v_encoder->get_frame(); 
    AVFrame *a_frame    =   a_encoder->get_frame();    
    AVFrame *frame      =   nullptr;
    Encode  *encoder    =   nullptr;

    AVRational st_tb;

    // flush function.
#if 0
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
#endif

#if 0
    // determine stream pts function
    auto order_pts_func = [&] () -> int
    {
        int     result  =   0;
        int64_t v_pts, a_pts;

        // 理論上不用考慮兩個都是 nullptr 的 case,在 loop 控制就排除這件事情了.
        if( v_frame == nullptr ) 
        {
            // flush video.
            // 必須在這邊處理,等loop完再處理有機會造成stream flush的部分寫入時間錯誤.  (例如聲音比影像短)
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
            // 原本想用 INT64_MAX, 但會造成 overflow.
            v_pts   =   v_frame->pts;
            a_pts   =   a_frame->pts;
            result  =   av_compare_ts( v_pts, v_time_base, a_pts, a_time_base );
        }

        return  result;
    };
#endif


    // 休息一下再來思考這邊怎麼改寫, 希望寫得好看一點
    while( v_frame != nullptr || a_frame != nullptr )
    {        
        assert( v_frame != nullptr || a_frame != nullptr );

        //
        //ret     =   order_pts_func();
        if( *v_encoder <= *a_encoder )
            ret =   0;
        else
            ret =   1;

        //
        if( ret <= 0 && v_frame != nullptr ) // video
        {
            encoder =   v_encoder;
            frame   =   v_frame;
            st_tb   =   muxer->get_video_stream_timebase();
        }
        else if( a_frame != nullptr ) // audio
        {
            encoder =   a_encoder;
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
        if( encoder == v_encoder ) // 理想是用 enum 處理, 這邊先偷懶, 有空修
            v_frame     =   encoder->get_frame();
        else
            a_frame     =   encoder->get_frame();

        if( v_frame == nullptr && v_encoder->is_flush() == false )
        {
            st_tb   =   muxer->get_video_stream_timebase();
            flush_encoder( v_encoder, &st_tb );
        }
        if( a_frame == nullptr && a_encoder->is_flush() == false )
        {
            st_tb   =   muxer->get_audio_stream_timebase();
            flush_encoder( a_encoder, &st_tb );
        }

    }
    
    // flush
    if( v_encoder->is_flush() == false )
    {
        st_tb   =   muxer->get_video_stream_timebase();
        flush_encoder( v_encoder, &st_tb );
    }

    if( a_encoder->is_flush() == false )
    {
        st_tb   =   muxer->get_audio_stream_timebase();
        flush_encoder( a_encoder, &st_tb );
    }

    //
    muxer->write_end();
}






/*******************************************************************************
Maker::work()
********************************************************************************/
void Maker::work()
{
    if( setting->has_subtitle == true )
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
    v_encoder->end();
    a_encoder->end();
    s_encoder->end();

    muxer->end();
    delete muxer;
    muxer   =   nullptr;
}







/*******************************************************************************
output_by_io
********************************************************************************/
void    output_by_io( MediaInfo media_info, std::string _port, Maker& maker )
{
    EncodeSetting   setting;
    setting.io_type         =   IO_Type::SRT_IO;    
    setting.srt_port        =   _port;
    setting.has_subtitle    =   false;

    VideoEncodeSetting  v_setting;
    v_setting.code_id   =   AV_CODEC_ID_H264;
    //v_setting.code_id   =   AV_CODEC_ID_H265;

    v_setting.width     =   media_info.width;
    v_setting.height    =   media_info.height;
    v_setting.pix_fmt   =   static_cast<AVPixelFormat>(media_info.pix_fmt);

    v_setting.time_base.num     =   media_info.time_num;
    v_setting.time_base.den     =   media_info.time_den;

    v_setting.gop_size      =   15;
    v_setting.max_b_frames  =   5;

    v_setting.src_width     =   media_info.width;
    v_setting.src_height    =   media_info.height;
    v_setting.src_pix_fmt   =   static_cast<AVPixelFormat>(media_info.pix_fmt);


    /*v_setting.width     =   1280;
    v_setting.height    =   720;
    v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P; 
    v_setting.time_base.num     =   1001; 
    v_setting.time_base.den     =   24000;

    v_setting.gop_size      =   12;
    v_setting.max_b_frames  =   0;

    v_setting.src_width     =   1280;
    v_setting.src_height    =   720; 
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGR24;
    
    v_setting.load_jpg_root_path    =   "J:\\jpg";*/


    AudioEncodeSetting  a_setting;
    a_setting.code_id           =   AV_CODEC_ID_AAC;
    a_setting.bit_rate          =   320000;
    a_setting.sample_rate       =   media_info.sample_rate;
    a_setting.channel_layout    =   media_info.channel_layout;
    a_setting.sample_fmt        =   media_info.sample_fmt;
    /*a_setting.sample_rate       =   48000;
    a_setting.channel_layout    =   3;
    a_setting.sample_fmt        =   AV_SAMPLE_FMT_S16;
    a_setting.load_pcm_path     =   "J:\\test.pcm";*/
  

    SubEncodeSetting   s_setting;

    maker.init( &setting, &v_setting, &a_setting, &s_setting );
    //maker.work_live_stream();
    maker.end();
}





/*******************************************************************************
maker_encode_example
********************************************************************************/
void    maker_encode_example()
{
    EncodeSetting   setting;
    setting.io_type =   IO_Type::DEFAULT;
    //setting.io_type =   IO_Type::FILE_IO;
    //setting.io_type =   IO_Type::SRT_IO;


    // rmvb 是 variable bitrate. 目前還無法使用
    //setting.filename    =   "H:\\output.mkv";
    //setting.extension   =   "matroska";
    setting.filename    =   "H:\\output.mp4";
    setting.extension   =   "mp4";
    //setting.filename    =   "H:\\test.avi"; 
    //setting.extension   =   "avi";
    //setting.srt_port        =   "1234";
    setting.has_subtitle    =   false;

    VideoEncodeSetting  v_setting;
    v_setting.load_jpg_root_path    =   "J:\\jpg";
    v_setting.code_id   =   AV_CODEC_ID_H264;
    //v_setting.code_id   =   AV_CODEC_ID_H265;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG1VIDEO;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG2VIDEO;

    v_setting.width     =   1280;
    v_setting.height    =   720;

    v_setting.time_base.num     =   1001;
    v_setting.time_base.den     =   24000;

    /*
        b frame not support on rm
    */
    v_setting.gop_size      =   30;
    v_setting.max_b_frames  =   15;
    //v_setting.gop_size      =   10;
    //v_setting.max_b_frames  =   0;


    v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P12LE;

    v_setting.src_width     =   1280;
    v_setting.src_height    =   720;
    //v_setting.src_pix_fmt   =   AV_PIX_FMT_BGRA;    // for QImage
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGR24;    // for openCV


    AudioEncodeSetting  a_setting;
    a_setting.load_pcm_path     =   "J:\\test.pcm";
    a_setting.code_id     =   AV_CODEC_ID_MP3;
    //a_setting.code_id       =   AV_CODEC_ID_AAC;
    //a_setting.code_id       =   AV_CODEC_ID_AC3;
    //a_setting.code_id     =   AV_CODEC_ID_MP2;
    //a_setting.code_id       =   AV_CODEC_ID_VORBIS;
    //a_setting.code_id       =   AV_CODEC_ID_FLAC;

    //a_setting.bit_rate          =   320000;
    a_setting.bit_rate          =   128000;
    a_setting.sample_rate       =   48000;
    a_setting.channel_layout    =   3; // AV_CH_LAYOUT_STEREO = 3;
    a_setting.sample_fmt        =   static_cast<int>(AV_SAMPLE_FMT_S16);

    SubEncodeSetting   s_setting;
    s_setting.code_id       =   AV_CODEC_ID_ASS;
    //s_setting.code_id       =   AV_CODEC_ID_SUBRIP;
    //s_setting.code_id       =   AV_CODEC_ID_MOV_TEXT;
    s_setting.subtitle_file =   "J:\\test.ass";
    s_setting.subtitle_ext  =   "ass";


    Maker   maker;

    maker.init( &setting, &v_setting, &a_setting, &s_setting );
    maker.work();
    //maker.work_live_stream();
    maker.end();
}





