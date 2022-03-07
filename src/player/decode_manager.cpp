#include "decode_manager.h"

#include "tool.h"
#include "video_decode.h"
#include "audio_decode.h"

extern "C" {

#include <libavformat/avformat.h>

} // end extern "C"





/*******************************************************************************
DecodeManager::DecodeManager()
********************************************************************************/
DecodeManager::DecodeManager()
{}








/*******************************************************************************
DecodeManager::~DecodeManager()
********************************************************************************/
DecodeManager::~DecodeManager()
{}





/*******************************************************************************
DecodeManager::get_current_video_decoder()
********************************************************************************/
VideoDecode*    DecodeManager::get_current_video_decoder()
{
    auto  itr  =   video_map.find(current_video_index);
    
    if( itr == video_map.end() )
    {
        MYLOG( LOG::L_ERROR, "current_video_index = %d, not found.", current_video_index );
        return  nullptr;
    }

    VideoDecode     *ptr    =   dynamic_cast<VideoDecode*>(itr->second);
    if( ptr == nullptr )
        MYLOG( LOG::L_ERROR, "get video decode pointer fail." );

    return  ptr;
}





/*******************************************************************************
DecodeManager::get_current_audio_decoder()
********************************************************************************/
AudioDecode*    DecodeManager::get_current_audio_decoder()
{
    auto  itr  =   audio_map.find(current_video_index);
    
    if( itr == audio_map.end() )
    {
        MYLOG( LOG::L_ERROR, "current_video_index = %d, not found.", current_video_index );
        return  nullptr;
    }

    AudioDecode     *ptr    =   dynamic_cast<AudioDecode*>(itr->second);
    if( ptr == nullptr )
        MYLOG( LOG::L_ERROR, "get video decode pointer fail." );

    return  ptr;
}





/*******************************************************************************
DecodeManager::exist_video_stream()
********************************************************************************/
bool    DecodeManager::exist_video_stream()
{
    return  video_map.empty() == false;
}





/*******************************************************************************
DecodeManager::exist_audio_stream()
********************************************************************************/
bool    DecodeManager::exist_audio_stream()
{
    return  audio_map.empty() == false;
}




/*******************************************************************************
DecodeManager::open_decoders()

ref: old code open_codec_context, open_all_codec
********************************************************************************/
int    DecodeManager::open_decoders( AVFormatContext* fmt_ctx )
{
    int     ret     =   0;
    int     index;

    // open video
    VideoDecode     *v_ptr  =   nullptr;
    for( index = 0; index < fmt_ctx->nb_streams; index++ )
    {
        ret  =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, index, -1, nullptr, 0 );

        if( ret >= 0 )
        {
            if( current_video_index < 0 )           
                current_video_index   =   index; // choose first as current.

            v_ptr   =   new VideoDecode;
            if( v_ptr == nullptr )
                MYLOG( LOG::L_ERROR, "alloc video decode fail." );

            video_map.emplace( index, dynamic_cast<Decode*>(v_ptr) );

            // note: dec_ctx, stream is class member. after open codec, they use for current ctx, stream.
            v_ptr->open_codec_context( index, fmt_ctx, AVMEDIA_TYPE_VIDEO );

            //dec_map.emplace(    std::make_pair(index,dec_ctx) ); 
            //stream_map.emplace( std::make_pair(index,stream)  );
        }
    }
    video_map[current_video_index]->set_is_current(true);

    // open audio
    AudioDecode     *a_ptr  =   nullptr;
    for( index = 0; index < fmt_ctx->nb_streams; index++ )
    {
        ret  =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_AUDIO, index, -1, nullptr, 0 );

        if( ret >= 0 )
        {
            if( current_audio_index < 0 )           
                current_audio_index   =   index; // choose first as current.

            a_ptr   =   new AudioDecode;
            if( a_ptr == nullptr )
                MYLOG( LOG::L_ERROR, "alloc audio decode fail." );

            audio_map.emplace( index, dynamic_cast<Decode*>(a_ptr) );
            a_ptr->open_codec_context( index, fmt_ctx, AVMEDIA_TYPE_AUDIO );
        }
    }
    audio_map[current_audio_index]->set_is_current(true);

    return  R_SUCCESS;
}




#ifdef FFMPEG_TEST
/*******************************************************************************
DecodeManager::set_output_jpg_root()
********************************************************************************/
void    DecodeManager::set_output_jpg_path( std::string _path )
{
    VideoDecode     *v_ptr  =   nullptr;
    for( auto itr : video_map )
    {
        v_ptr   =   dynamic_cast<VideoDecode*>(itr.second);
        assert( v_ptr != nullptr );
        v_ptr->set_output_jpg_path(_path);
    }

    //s_decoder.set_output_jpg_root(_root_path);
}
#endif





#ifdef FFMPEG_TEST
/*******************************************************************************
DecodeManager::set_output_audio_pcm_path()
********************************************************************************/
void    DecodeManager::set_output_audio_pcm_path( std::string _path )
{
    AudioDecode     *a_ptr  =   nullptr;
    for( auto itr : audio_map )
    {
        a_ptr   =   dynamic_cast<AudioDecode*>(itr.second);
        assert( a_ptr != nullptr );
        a_ptr->set_output_audio_pcm_path(_path);
    }
}
#endif





/*******************************************************************************
DecodeManager::init_decoders()
********************************************************************************/
void    DecodeManager::init_decoders()
{
    // video
    VideoDecode *v_ptr  =   nullptr;
    for( auto itr : video_map )
    {
        v_ptr   =   dynamic_cast<VideoDecode*>(itr.second);
        v_ptr->init();
    }

    // audio
    AudioDecode *a_ptr  =   nullptr;
    for( auto itr : audio_map )
    {
        a_ptr   =   dynamic_cast<AudioDecode*>(itr.second);
        a_ptr->init();
    }
}





/*******************************************************************************
DecodeManager::get_decoder()
********************************************************************************/
Decode*     DecodeManager::get_decoder( int stream_index )
{
    // video
    auto    v_itr  =   video_map.find(stream_index);
    if( v_itr != video_map.end() )
        return  v_itr->second;

    // audio
    auto    a_itr  =   audio_map.find(stream_index);
    if( a_itr != audio_map.end() )
        return  a_itr->second;

    // subtitle
    auto    s_itr  =   subtitle_map.find(stream_index);
    if( s_itr != subtitle_map.end() )
        return  s_itr->second;

    MYLOG( LOG::L_ERROR, "decode not found." );
    return  nullptr;
}




/*******************************************************************************
DecodeManager::flush_decoders()
********************************************************************************/
void    DecodeManager::flush_decoders()
{
#if 0

    // flush
    // subtitle 必須在最前面 flush.
    if( s_decoder.exist_stream() == true )
        s_decoder.flush();
#ifdef RENDER_SUBTITLE
    /* 
        在有字幕的情況下, video decoder 需要額外呼叫 subtitle decoer 來處理, 所以需要額外的 code.
        如果要併入 flush, 設計上並沒有比較好看.
    */
    ret     =   v_decoder.send_packet(nullptr);
    if( ret >= 0 )
    {       
        while(true)
        {
            ret     =   v_decoder.recv_frame(-1);
            if( ret <= 0 )
                break;

            if( v_decoder.output_frame_func != nullptr )
                v_decoder.output_frame_func();
            v_decoder.unref_frame();
        }
    }
#else
    // 理論上只有一個 video stream. 如果不是, 這邊有機會出問題.
    v_decoder.flush();
#endif

    a_decoder.flush();

#endif

    // subtitle 必須在最前面 flush.
    for( auto itr : subtitle_map )
        itr.second->flush();

    for( auto itr : video_map )
        itr.second->flush();

    for( auto itr : audio_map )
        itr.second->flush();
}
