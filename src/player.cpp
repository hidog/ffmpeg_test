#include "player.h"
#include "tool.h"


extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

} // end extern "C"


std::queue<AudioData> audio_queue;
std::queue<VideoData> video_queue;

std::queue<AudioData>* get_audio_queue()
{
    return  &audio_queue;
}

std::queue<VideoData>* get_video_queue()
{
    return &video_queue;
}




/*******************************************************************************
Player::Player()
********************************************************************************/
DLL_API Player::Player()
{
    src_filename        =   "D:\\code\\test.mp4";
    video_dst_filename  =   "I:\\v.data";
    audio_dst_filename  =   "I:\\a.data";
}




/*******************************************************************************
Player::~Player()
********************************************************************************/
DLL_API Player::~Player()
{}




/*******************************************************************************
Player::init()
********************************************************************************/
int     Player::init()
{
    int     ret     =   -1;
    int     vs_idx  =   -1;
    int     as_idx  =   -1;

    AVFormatContext *fmt_ctx    =   nullptr;

    //
    demuxer.open_input( src_filename );
    demuxer.init();
    fmt_ctx =   demuxer.get_format_context();
    vs_idx  =   demuxer.get_video_index();
    as_idx  =   demuxer.get_audio_index();

    //
    //v_decoder.open_output(video_dst_filename);
    //a_decoder.open_output(audio_dst_filename);

    v_decoder.open_codec_context( vs_idx, fmt_ctx );
    a_decoder.open_codec_context( as_idx, fmt_ctx );

    v_decoder.init();
    a_decoder.init();

    return SUCCESS;
}



/*******************************************************************************
Player::play()
********************************************************************************/
void    Player::play()
{
    int         count   =   0;
    int         ret     =   0;
    AVPacket*   pkt     =   nullptr;
    Decode      *dc     =   nullptr;
    AVFrame     *frame  =   nullptr;

    int v_count = 0, a_count = 0;

    //
    printf( "Demuxing video from file '%s' into '%s'\n", src_filename.c_str(), video_dst_filename.c_str() );
    printf( "Demuxing audio from file '%s' into '%s'\n", src_filename.c_str(), audio_dst_filename.c_str() );

    // 如何直接輸出 packet 的範例
    //fwrite( pkt->data, pkt->size, 1, fp );
    //printf("write video data %d bytes.\n", pkt->size);

    // read frames from the file 
    while( true ) 
    {
        //printf("count = %d\n", count++);

        ret = demuxer.demux();
        if( ret < 0 )
        {
            printf("play end.\n");
            break;
        }
        
        pkt     =   demuxer.get_packet();
        if( pkt->stream_index == demuxer.get_video_index() )
            dc  =   dynamic_cast<Decode*>(&v_decoder);        
        else if( pkt->stream_index == demuxer.get_audio_index() )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else        
            ERRLOG("stream type not handle.");        

        //
        ret     =   dc->send_packet(pkt);
        if( ret >= 0 )
        {
            while(true)
            {
                ret     =   dc->recv_frame();
                if( ret <= 0 )
                    break;

                // 可以取得frame,並對資訊做輸出. 實際上這部分操作被包在decoder內.
                // frame   =   dc->get_frame(); 

                if( dc->get_type() == static_cast<myAVMediaType>(AVMEDIA_TYPE_VIDEO) )
                    v_count++;
                else
                    a_count++;

                printf("v count = %d a_count = %d\n", v_count, a_count );


                dc->output_frame_func();
                dc->unref_frame();
            }
        }

        //demuxer.unref_packet();
    }

    v_decoder.flush();
    a_decoder.flush();

    printf("Demuxing succeeded.\n");

    printf("v count = %d a count = %d\n", v_count, a_count );


    //
    v_decoder.print_finish_message();
    a_decoder.print_finish_message();
}





/*******************************************************************************
Player::pause()
********************************************************************************/
void    Player::pause()
{
    is_pause    =   true;
}






/*******************************************************************************
Player::resume()
********************************************************************************/
void    Player::resume()
{
    is_pause    =   false;
}


#include <thread>


/*******************************************************************************
Player::play_QT()
********************************************************************************/
void    Player::play_QT()
{
    int         count   =   0;
    int         ret     =   0;
    AVPacket*   pkt     =   nullptr;
    Decode      *dc     =   nullptr;
    AVFrame     *frame  =   nullptr;

    int v_count = 0;

    // read frames from the file 
    while( true ) 
    {

        //while( audio_queue.size() > 120 || video_queue.size() > 40 )
        while( video_queue.size() > 40 )
        {
            //printf("wait...\n");
            std::this_thread::sleep_for( std::chrono::milliseconds(1) );
        }

        //printf("a queue %d, v queue %d\n", audio_queue.size(), video_queue.size() );


        ret = demuxer.demux();
        if( ret < 0 )
            break;        

        pkt     =   demuxer.get_packet();
        if( pkt->stream_index == demuxer.get_video_index() )
            dc  =   dynamic_cast<Decode*>(&v_decoder);        
        else if( pkt->stream_index == demuxer.get_audio_index() )
            dc  =   dynamic_cast<Decode*>(&a_decoder);
        else        
            ERRLOG("stream type not handle.");

        //
        ret     =   dc->send_packet(pkt);
        if( ret >= 0 )
        {
            while(true)
            {
                ret     =   dc->recv_frame();
                if( ret <= 0 )
                    break;

                if( pkt->stream_index == demuxer.get_video_index() )
                {
                    //QImage frame = v_decoder.output_QImage();
                    //output_video_frame_func(frame);
                    VideoData data;
                    data.index = v_count++;
                    data.frame = v_decoder.output_QImage();
                    data.ts = v_decoder.get_ts();
                    video_queue.push(data);
                }
                else if( pkt->stream_index == demuxer.get_audio_index() )
                {
                    AudioData ad = a_decoder.output_PCM();
                    //output_audio_pcm_func(ad);
                    audio_queue.push(ad);
                }
                else
                    assert(0);

                //std::this_thread::sleep_for( std::chrono::milliseconds(10) );

                //dc->output_frame_func();
                dc->unref_frame();
            }
        }

        //demuxer.unref_packet();
    }

    v_decoder.flush();
    a_decoder.flush();
}







/*******************************************************************************
Player::end()
********************************************************************************/
int     Player::end()
{
    v_decoder.end();
    a_decoder.end();
    demuxer.end();

    return  SUCCESS;
}
