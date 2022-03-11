#include "decode_manager.h"

#include "tool.h"
#include "video_decode.h"
#include "audio_decode.h"
#include "sub_decode.h"

#include <sstream>


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

    VideoDecode     *ptr    =   itr->second;
    if( ptr == nullptr )
        MYLOG( LOG::L_ERROR, "get video decode pointer fail." );

    return  ptr;
}





/*******************************************************************************
DecodeManager::get_current_audio_decoder()
********************************************************************************/
AudioDecode*    DecodeManager::get_current_audio_decoder()
{
    auto  itr  =   audio_map.find(current_audio_index);
    
    if( itr == audio_map.end() )
    {
        MYLOG( LOG::L_ERROR, "current_audio_index = %d, not found.", current_audio_index );
        return  nullptr;
    }

    AudioDecode     *ptr    =   itr->second;
    if( ptr == nullptr )
        MYLOG( LOG::L_ERROR, "get audio decode pointer fail." );

    return  ptr;
}






/*******************************************************************************
DecodeManager::get_current_subtitle_decoder()
********************************************************************************/
SubDecode*      DecodeManager::get_current_subtitle_decoder()
{
    auto  itr  =   subtitle_map.find(current_subtitle_index);
    
    if( itr == subtitle_map.end() )
    {
        MYLOG( LOG::L_ERROR, "current_subtitle_index = %d, not found.", current_subtitle_index );
        return  nullptr;
    }

    SubDecode     *ptr    =   itr->second;
    if( ptr == nullptr )
        MYLOG( LOG::L_ERROR, "get subtitle decode pointer fail." );

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
        // note: video stream 有機會是音檔內的封面圖片
        ret  =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, index, -1, nullptr, 0 );

        if( ret >= 0 )
        {
            if( current_video_index < 0 )           
                current_video_index   =   index; // choose first as current.

            v_ptr   =   get_video_decoder_instance();
            if( v_ptr == nullptr )
                MYLOG( LOG::L_ERROR, "alloc video decode fail." );

            video_map.emplace( index, v_ptr );

            // note: dec_ctx, stream is class member. after open codec, they use for current ctx, stream.
            v_ptr->open_codec_context( index, fmt_ctx, AVMEDIA_TYPE_VIDEO );
        }

    }
    if( current_video_index >= 0 )
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

            audio_map.emplace( index, a_ptr );
            a_ptr->open_codec_context( index, fmt_ctx, AVMEDIA_TYPE_AUDIO );
        }
    }
    if( current_audio_index >= 0 )
        audio_map[current_audio_index]->set_is_current(true);

    // open subtitle
    SubDecode   *s_ptr      =   nullptr;
    for( index = 0; index < fmt_ctx->nb_streams; index++ )
    {
        ret  =   av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_SUBTITLE, index, -1, nullptr, 0 );

        if( ret >= 0 )
        {
            if( current_subtitle_index < 0 )           
                current_subtitle_index   =   index;

            s_ptr   =   new SubDecode;
            if( s_ptr == nullptr )
                MYLOG( LOG::L_ERROR, "alloc subtitle decode fail." );

            subtitle_map.emplace( index, s_ptr );
            s_ptr->open_codec_context( index, fmt_ctx, AVMEDIA_TYPE_SUBTITLE );
        }
    }
    if( current_subtitle_index >= 0 )
        subtitle_map[current_subtitle_index]->set_is_current(true);

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
        v_ptr   =   itr.second;
        assert( v_ptr != nullptr );
        v_ptr->set_output_jpg_path(_path);
    }

    //s_decoder.set_output_jpg_root(_root_path);
    SubDecode   *s_ptr  =   nullptr;
    for( auto itr : subtitle_map )
    {
        s_ptr  =   itr.second;
        assert( s_ptr != nullptr );
        s_ptr->set_output_jpg_root(_path);
    }
}
#endif




/*******************************************************************************
DecodeManager::is_video_attach()
********************************************************************************/
bool    DecodeManager::is_video_attachd()
{
    if( exist_video_stream() == false )
    {
        MYLOG( LOG::L_WARN, "no video stream." )
        return  false;
    }
    else
    {
        VideoDecode *ptr    =   get_current_video_decoder();
        assert( ptr != nullptr );
        return  ptr->is_attached();
    }
}




#ifdef FFMPEG_TEST
/*******************************************************************************
DecodeManager::set_output_audio_pcm_path()
********************************************************************************/
void    DecodeManager::set_output_audio_pcm_path( std::string _path )
{
    AudioDecode     *a_ptr  =   nullptr;
    for( auto itr : audio_map )
    {
        a_ptr   =   itr.second;
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
        v_ptr   =   itr.second;
        v_ptr->init();
    }

    // audio
    AudioDecode *a_ptr  =   nullptr;
    for( auto itr : audio_map )
    {
        a_ptr   =   itr.second;
        a_ptr->init();
    }

    // subtitle
    SubDecode   *s_ptr  =   nullptr;
    for( auto itr : subtitle_map )
    {
        s_ptr   =   itr.second;
        s_ptr->init();
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





#ifdef FFMPEG_TEST
/*******************************************************************************
DecodeManager::flush_decoders()
********************************************************************************/
void    DecodeManager::flush_decoders()
{
    // subtitle 必須在最前面 flush.
    for( auto itr : subtitle_map )
        itr.second->flush();

    for( auto itr : video_map )
        itr.second->flush();

    for( auto itr : audio_map )
        itr.second->flush();
}
#endif







/*******************************************************************************
DecodeManager::get_current_video_index()
********************************************************************************/
int     DecodeManager::get_current_video_index()
{
    return  current_video_index;
}





    
/*******************************************************************************
DecodeManager::get_current_audio_index()
********************************************************************************/
int     DecodeManager::get_current_audio_index()
{
    return  current_audio_index;
}





/*******************************************************************************
DecodeManager::get_current_subtitle_index()
********************************************************************************/
int     DecodeManager::get_current_subtitle_index()
{
    return  current_subtitle_index;
}






/*******************************************************************************
DecodeManager::exist_subtitle_stream()
********************************************************************************/
bool    DecodeManager::exist_subtitle_stream()
{
    return  subtitle_map.empty() == false;
}




/*******************************************************************************
DecodeManager::set_subtitle_file()
********************************************************************************/
void    DecodeManager::set_subtitle_file( std::string path )
{
    subtitle_file   =   path;
}




/*******************************************************************************
DecodeManager::set_sub_src_type()

NOTE: 如果需要同時支援 ass file 跟 media file 內的 subtitle, 
      需要將相關資訊搬到 SubDecode 底下.
********************************************************************************/
void    DecodeManager::set_sub_src_type( SubSourceType type )
{
    sub_src_type    =   type;
}





/*******************************************************************************
DecodeManager::switch_subtltle()
********************************************************************************/
void    DecodeManager::switch_subtltle( std::string path )
{
    set_subfile( path );

    std::string     filename    =   "\\";    
    filename    +=  subtitle_file;
    filename.insert( 2, 1, '\\' );

    std::stringstream   ss;
    ss << "subtitles='" << filename << "':stream_index=" << 0;

    std::string     desc    =   ss.str();

    SubDecode*  s_ptr   =   get_current_subtitle_decoder();
    s_ptr->open_subtitle_filter( subtitle_args, desc );
}




/*******************************************************************************
DecodeManager::switch_subtltle()
********************************************************************************/
void    DecodeManager::switch_subtltle( int index )
{
    SubDecode*      s_ptr   =   get_current_subtitle_decoder();
    std::string     desc;

    if( s_ptr->is_graphic_subtitle() == false )
    {
        subtitle_index   =   index;

        std::string     filename    =   "\\";    
        filename    +=  subtitle_file;
        filename.insert( 2, 1, '\\' );

        std::stringstream   ss;
        ss << "subtitles='" << filename << "':stream_index=" << subtitle_index;

        desc    =   ss.str();
        s_ptr->open_subtitle_filter( subtitle_args, desc );
    }

}






/*******************************************************************************
DecodeManager::get_sub_src_type()
********************************************************************************/
SubSourceType   DecodeManager::get_sub_src_type()
{
    return  sub_src_type;
}






/*******************************************************************************
DecodeManager::init_subtitle()
********************************************************************************/
void    DecodeManager::init_subtitle( AVFormatContext *fmt_ctx, DecodeSetting setting )
{
    int             ret;
    bool            exist_subtitle  =   false;
    SubData         sd;

    std::pair<std::string,std::string>  sub_param;

    //if( s_decoder.exist_stream() == true )
    if( exist_subtitle_stream() == true )
    {
        exist_subtitle  =   true;
        set_subtitle_file( setting.filename );
        set_sub_src_type( SubSourceType::EMBEDDED );
    }
    else
    {
        if( setting.subname.empty() == false )
        {
            // create subtitle decode for file.
            SubDecode   *s_ptr  =   new SubDecode;
            s_ptr->init();
            subtitle_map.emplace( file_subtitle_index, s_ptr );
            current_subtitle_index  =   file_subtitle_index;

            exist_subtitle  =   true;
            set_subtitle_file( setting.subname );
            set_sub_src_type( SubSourceType::FROM_FILE );
        }
        else
        {
            exist_subtitle  =   false;
            set_sub_src_type( SubSourceType::NONE );
        }
    }

    // 
    if( exist_subtitle == true )
    {
        // note: 如果遇到同時有 audio stream, sub stream, 但沒有 video stream, 會crash. 但理論上不會有這種case.
        VideoDecode *v_ptr  =   get_current_video_decoder();

        sd.width        =   v_ptr->get_video_width();
        sd.height       =   v_ptr->get_video_height();
        sd.pix_fmt      =   v_ptr->get_pix_fmt();
        sd.video_index  =   current_video_index;
        sd.sub_index    =   0;

        for( auto itr : subtitle_map )
        {
            SubDecode   *s_ptr  =   itr.second;

            if( s_ptr->is_graphic_subtitle() == true )
                s_ptr->init_graphic_subtitle(sd);        
            else
            {
                s_ptr->init_sws_ctx( sd );

                // if exist subtitle, open it.
                // 這邊有執行順序問題, 不能隨便更改執行順序      
                sub_param   =   get_subtitle_param( fmt_ctx, subtitle_file, sd );
                s_ptr->open_subtitle_filter( sub_param.first, sub_param.second );
                subtitle_args   =   sub_param.first;
            }       
        }

#if defined(RENDER_SUBTITLE) || !defined(FFMPEG_TEST)
        // 若有 subtitle, 設置進去 video decoder.
        if( exist_subtitle_stream() == true )
        {
            SubDecode   *s_ptr  =   get_current_subtitle_decoder();
            v_ptr->set_subtitle_decoder(s_ptr);
        }
#endif

    }
}







/*******************************************************************************
DecodeManager::get_subtitle_param()
********************************************************************************/
std::pair<std::string,std::string>  DecodeManager::get_subtitle_param( AVFormatContext* fmt_ctx, std::string src_file, SubData sd )
{
    SubDecode   *s_ptr  =   get_current_subtitle_decoder();

    if( s_ptr->is_graphic_subtitle() == true )
        MYLOG( LOG::L_ERROR, "cant handle graphic subtitle." );

    std::stringstream   ss;
    std::string     in_param, out_param;;
    
    AVRational  frame_rate  =   av_guess_frame_rate( fmt_ctx, fmt_ctx->streams[sd.video_index], NULL );

    int     sar_num     =   fmt_ctx->streams[sd.video_index]->codecpar->sample_aspect_ratio.num; // old code use fmt_ctx->streams[sd.video_index]->sample_aspect_ratio.num
    int     sar_den     =   FFMAX( fmt_ctx->streams[sd.video_index]->codecpar->sample_aspect_ratio.den, 1 );

    int     tb_num      =   fmt_ctx->streams[sd.video_index]->time_base.num;
    int     tb_den      =   fmt_ctx->streams[sd.video_index]->time_base.den;

    ss << "video_size=" << sd.width << "x" << sd.height << ":pix_fmt=" << static_cast<int>(sd.pix_fmt) 
       << ":time_base=" << tb_num << "/" << tb_den << ":pixel_aspect=" << sar_num << "/" << sar_den;

    if( frame_rate.num != 0 && frame_rate.den != 0 )
        ss << ":frame_rate=" << frame_rate.num << "/" << frame_rate.den;

    in_param   =   ss.str();

    MYLOG( LOG::L_INFO, "in = %s", in_param.c_str() );

    ss.str("");
    ss.clear();   

    // make filename param. 留意絕對路徑的格式, 不能亂改, 會造成錯誤.
    std::string     filename_param  =   "\\";
    filename_param  +=  src_file;
    filename_param.insert( 2, 1, '\\' );
    
    // 理論上這邊的字串可以精簡...
    subtitle_index   =   sd.sub_index;
    ss << "subtitles='" << filename_param << "':stream_index=" << subtitle_index;
    
    out_param    =   ss.str();
    //out_param    =   "subtitles='\\D\\:/code/test.mkv':stream_index=0";

    MYLOG( LOG::L_INFO, "out = %s", out_param.c_str() );
    return  std::make_pair( in_param, out_param );
}









/*******************************************************************************
DecodeManager::flush_decoders_for_seek()
********************************************************************************/
void    DecodeManager::flush_decoders_for_seek()
{
    for( auto itr : video_map )    
        itr.second->flush_for_seek();
    
    for( auto itr : audio_map )
        itr.second->flush_for_seek();

    for( auto itr : subtitle_map )
        itr.second->flush_for_seek();
}



/*******************************************************************************
DecodeManager::get_embedded_subtitle_list().
********************************************************************************/
std::vector<std::string>    DecodeManager::get_embedded_subtitle_list()
{
    std::vector<std::string>    list;

    /*char    *buf    =   nullptr;
    av_dict_get_string( stream->metadata, &buf, '=', ',' );
    MYLOG( LOG::L_INFO, "buf = %s\n", buf );*/

    AVStream    *st     =   nullptr;
    SubDecode   *s_ptr  =   nullptr;
    for( auto itr : subtitle_map )
    {
        s_ptr   =   itr.second;
        st      =   s_ptr->get_stream();

        AVDictionaryEntry   *dic   =   av_dict_get( (const AVDictionary*)st->metadata, "title", NULL, AV_DICT_MATCH_CASE );
        if( dic != nullptr )
        {
            MYLOG( LOG::L_DEBUG, "title %s", dic->value );
            list.emplace_back( std::string(dic->value) );
        }
        else
            list.emplace_back( std::string("default") );  // 遇到多字幕都沒有定義 title 再來調整這裡的程式碼...
    }

    return  list;
}




/*******************************************************************************
DecodeManager::release().
********************************************************************************/
void    DecodeManager::release()
{
    for( auto itr : video_map )
        delete  itr.second;
    video_map.clear();

    for( auto itr : audio_map )
        delete  itr.second;
    audio_map.clear();

    for( auto itr : subtitle_map )
        delete  itr.second;
    subtitle_map.clear();

    current_video_index     =   -1;
    current_audio_index     =   -1;
    current_subtitle_index  =   -1;
    sub_src_type    =   SubSourceType::NONE;
}




/*******************************************************************************
DecodeManager::flush_all_sub_stream().
********************************************************************************/
void    DecodeManager::flush_all_sub_stream()
{
    int     ret     =   0;
    int     got_sub =   0;

    // for subtitle flush, data = null, size = 0. 
    // 這邊可以省略 pkt 的初始化
    AVPacket    pkt;
    pkt.data    =   nullptr;
    pkt.size    =   0;

    AVSubtitle      subtitle;

    SubDecode       *s_ptr  =   nullptr;
    AVCodecContext  *ctx    =   nullptr;

    for( auto dec : subtitle_map )
    {
        if( dec.first == file_subtitle_index ) // 字幕檔不需要flush.
            continue;

        while(true)
        {
            s_ptr   =   dec.second;
            ctx     =   s_ptr->get_decode_context();
            ret     =   avcodec_decode_subtitle2( ctx, &subtitle, &got_sub, &pkt );
            if( ret < 0 )
                MYLOG( LOG::L_ERROR, "flush decode subtitle fail." );

            avsubtitle_free(&subtitle);

            if( got_sub <= 0 )
                break;
            
            if( dec.first == current_subtitle_index && subtitle.format == 0 )       
                s_ptr->generate_subtitle_image( subtitle );
        }
    }
}





/*******************************************************************************
DecodeManager::flush_all_video_stream().
********************************************************************************/
void    DecodeManager::flush_video()
{
    int     ret;
    VideoDecode     *v_ptr  =   nullptr;

    for( auto dec : video_map )
    {
        if( dec.first == current_video_index )
            continue;

        v_ptr   =   dec.second;
        v_ptr->send_packet(nullptr);

        while(true)
        {
            ret     =   v_ptr->recv_frame(-1);
            if( ret <= 0 )
                break;
            v_ptr->unref_frame();
        }
    }
}




/*******************************************************************************
DecodeManager::flush_all_video_stream().
********************************************************************************/
void    DecodeManager::flush_audio()
{
    int     ret;
    AudioDecode     *a_ptr  =   nullptr;

    for( auto dec : audio_map )
    {
        if( dec.first == current_audio_index )
            continue;

        a_ptr   =   dec.second;
        a_ptr->send_packet(nullptr);

        while(true)
        {
            ret     =   a_ptr->recv_frame(-1);
            if( ret <= 0 )
                break;
            a_ptr->unref_frame();
        }
    }
}






/*******************************************************************************
DecodeManager::set_subfile()
********************************************************************************/
void    DecodeManager::set_subfile( std::string path )
{
    subtitle_file    =   path;

#ifdef _WIN32
    // 斜線會影響執行結果.
    for( auto &c : subtitle_file )
    {
        if( c == '\\' )
            c   =   '/';
    }
#endif
}





/*******************************************************************************
DecodeManager::index_is_available()
檢查 pkg stream_index 是否合法
********************************************************************************/
bool    DecodeManager::index_is_available( int index )
{
    auto    sdec    =   subtitle_map.find(index);
    if( sdec != subtitle_map.end() )
        return  true;

    auto    adec    =   audio_map.find(index);
    if( adec != audio_map.end() )
        return  true;

    auto    vdec    =   video_map.find(index);
    if( vdec != video_map.end() )
        return  true;

    return  false;
}


