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

    // set
    //dec_ctx     =   cs_index == -1 ? nullptr : dec_map[cs_index];
    //stream      =   cs_index == -1 ? nullptr : stream_map[cs_index];

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

            // note: dec_ctx, stream is class member. after open codec, they use for current ctx, stream.
            a_ptr->open_codec_context( index, fmt_ctx, AVMEDIA_TYPE_AUDIO );

            //dec_map.emplace(    std::make_pair(index,dec_ctx) ); 
            //stream_map.emplace( std::make_pair(index,stream)  );
        }
    }

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