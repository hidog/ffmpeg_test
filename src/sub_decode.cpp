#include "sub_decode.h"
#include "tool.h"
#include <sstream>


extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libavfilter/buffersrc.h>  // use for subtitle
#include <libavfilter/buffersink.h>

#include <libswscale/swscale.h>


} // end extern "C"






/*******************************************************************************
SubDecode::SubDecode()
********************************************************************************/
SubDecode::SubDecode()
    :   Decode()
{
    type  =   AVMEDIA_TYPE_SUBTITLE; 
}









/*******************************************************************************
SubDecode::get_subtitle_param()
********************************************************************************/
std::pair<std::string,std::string>  SubDecode::get_subtitle_param( AVFormatContext* fmt_ctx, int v_index, int width, int height, std::string src_file, AVPixelFormat pix_fmt, int current_subtitle_index )
{
    std::stringstream   ss;
    std::string     in_param, out_param;;

    int     sar_num     =   fmt_ctx->streams[v_index]->sample_aspect_ratio.num;
    int     sar_den     =   fmt_ctx->streams[v_index]->sample_aspect_ratio.den;

    int     tb_num      =   fmt_ctx->streams[v_index]->time_base.num;
    int     tb_den      =   fmt_ctx->streams[v_index]->time_base.den;

    ss << "video_size=" << width << "x" << height << ":pix_fmt=" << static_cast<int>(pix_fmt) 
       << ":time_base=" << tb_num << "/" << tb_den << ":pixel_aspect=" << sar_num << "/" << sar_den;

    in_param   =   ss.str();

    MYLOG( LOG::INFO, "in = %s", in_param.c_str() );

    ss.str("");
    ss.clear();   

    // make filename param. �d�N������|���榡, ����ç�, �|�y�����~.
    // �o��ݭn�[�J�P�_, �p�G�ɮ��ح����r���y, �N�}���ɮ�. �p�G�S��, �N�j�M�ö}�� subtitle.   
    std::string     filename_param  =   "\\";
    filename_param  +=  src_file;
    filename_param.insert( 2, 1, '\\' );

    ss << "subtitles=filename='" << filename_param << "':original_size=" << width 
       << "x" << height << ":stream_index=" << current_subtitle_index;

    out_param    =   ss.str();

    MYLOG( LOG::INFO, "out = %s", out_param.c_str() );


    return  std::make_pair( in_param, out_param );
}







/*******************************************************************************
SubDecode::output_decode_info()
********************************************************************************/
void    SubDecode::output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx )
{
    MYLOG( LOG::INFO, "sub dec name = %s", dec->name );
    MYLOG( LOG::INFO, "sub dec long name = %s", dec->long_name );
    MYLOG( LOG::INFO, "sub dec codec id = %s", avcodec_get_name(dec->id) );
    //MYLOG( LOG::INFO, "audio bitrate = %d", dec_ctx->bit_rate );
}




/*******************************************************************************
SubDecode::~SubDecode()
********************************************************************************/
SubDecode::~SubDecode()
{}





/*******************************************************************************
SubDecode::open_codec_context()
********************************************************************************/
int     SubDecode::open_codec_context( AVFormatContext *fmt_ctx )
{
    Decode::open_all_codec( fmt_ctx, type );
    //dec_ctx->thread_count = 4;
    return  SUCCESS;
}





/*******************************************************************************
SubDecode::init()
********************************************************************************/
int     SubDecode::init()
{
    filterGraph = avfilter_graph_alloc();

    Decode::init();
    return  SUCCESS;
}




/*******************************************************************************
SubDecode::send_video_frame()
********************************************************************************/
int SubDecode::send_video_frame( AVFrame *video_frame )
{
    int ret =   av_buffersrc_add_frame_flags( buffersrcContext, video_frame, AV_BUFFERSRC_FLAG_KEEP_REF );    
    if( ret < 0 )    
        MYLOG( LOG::ERROR, "add frame flag fail." );
    return  ret;
}



/*******************************************************************************
SubDecode::flush()
********************************************************************************/
int SubDecode::flush( AVFrame *video_frame )
{
    int ret =   av_buffersrc_add_frame_flags( buffersrcContext, video_frame, AV_BUFFERSRC_FLAG_PUSH );    
    if( ret < 0 )    
        MYLOG( LOG::ERROR, "add frame flag fail." );
    return  ret;
}





/*******************************************************************************
SubDecode::init_sub_image()
********************************************************************************/
void    SubDecode::init_sub_image( int width, int height )
{
    sub_image  =   QImage{ width, height, QImage::Format_RGB888 };  // ���ӦҼ{�����o�䪺code, �קK���ƪ�l��.
}





/*******************************************************************************
SubDecode::init_sws_ctx()
********************************************************************************/
int SubDecode::init_sws_ctx( int width, int height, AVPixelFormat pix_fmt )
{
    sws_ctx     =   sws_getContext( width, height, pix_fmt, width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);   
    return 1;
}



/*******************************************************************************
SubDecode::render_subtitle()
********************************************************************************/
int SubDecode::render_subtitle()
{
    int ret     =   av_buffersink_get_frame( buffersinkContext, frame );    
    if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ) 
        return  0;  // �S���,���S���~.
    else if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "get frame fail." );
        return  -1;
    }

    // render subtitle and generate image.
    uint8_t *dst[]  =   { sub_image.bits() };
    int     linesizes[4];

    //
    av_image_fill_linesizes( linesizes, AV_PIX_FMT_RGB24, frame->width );
    sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, dst, linesizes );

    //av_frame_unref(frame);
    return 1;
}





/*******************************************************************************
SubDecode::end()
********************************************************************************/
int     SubDecode::end()
{
    avfilter_free( buffersrcContext );
    avfilter_free( buffersinkContext );
    sws_freeContext( sws_ctx );
    avfilter_graph_free(&filterGraph);


    Decode::end();
    return  SUCCESS;
}



/*******************************************************************************
SubDecode::output_audio_frame_info()
********************************************************************************/
void    SubDecode::output_sub_frame_info()
{
    /*char    buf[AV_TS_MAX_STRING_SIZE]{0};
    int     per_sample  =   av_get_bytes_per_sample( static_cast<AVSampleFormat>(frame->format) );
    auto    pts_str     =   av_ts_make_time_string( buf, frame->pts, &dec_ctx->time_base );
    MYLOG( LOG::INFO, "audio_frame n = %d, nb_samples = %d, pts : %s", frame_count++, frame->nb_samples, pts_str );*/
}





/*******************************************************************************
SubDecode::output_audio_data()
********************************************************************************/
SubData   SubDecode::output_sub_data()
{
#if 0
    AudioData   ad { nullptr, 0 };

    // ���Ũӭק�o�� �n��ʺA�ھ� mp4 �ɮװ��վ�

    uint8_t     *data[2]    =   { 0 };  // S16 �� S32, ���T�w�O���O�o�䪺 array �n�令 4
    int         byte_count     =   frame->nb_samples * 2 * 2;  // S16 �� S32, �令 *4, �z�פW��ƶq�|�W�[, �����T�w�O�_�諸�O�o��

    unsigned char   *pcm    =   new uint8_t[byte_count];     // frame->nb_samples * 2 * 2     ���     ���t�˥���ƶq * ��q�D * �C�q�D2�줸�դj�p

    if( pcm == nullptr )
        MYLOG( LOG::WARN, "pcm is null" );

    data[0]     =   pcm;    // ��X�榡��AV_SAMPLE_FMT_S16(packet���O),�ҥH�ഫ�᪺ LR ��q�D���s�bdata[0]��
    int ret     =   swr_convert( swr_ctx,
                                 data, sample_rate,                                    //��X
                                 //data, frame->nb_samples,                              //��X
                                 (const uint8_t**)frame->data, frame->nb_samples );    //��J

    ad.pcm      =   pcm;
    ad.bytes    =   byte_count;

    return ad;
#endif
    SubData     sd { 0 };
    return  sd;
}








/*******************************************************************************
SubDecode::init_subtitle_filter()
********************************************************************************/
bool SubDecode::open_subtitle_filter( std::string args, std::string filterDesc)
{
    int     ret     =   0;

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *output = avfilter_inout_alloc();
    AVFilterInOut *input = avfilter_inout_alloc();
    //AVFilterGraph *filterGraph = avfilter_graph_alloc();
    //filterGraph = avfilter_graph_alloc();

    // lambda operator, �ٲ��F�ǤJ�ѼƬA��. 
    auto release = [&output, &input] 
    {
        avfilter_inout_free(&output);
        avfilter_inout_free(&input);
        //avfilter_graph_free(&filterGraph);
    };

    if (!output || !input || !filterGraph) 
    {
        release();
        return false;
    }

    // Crear filtro de entrada, necesita arg
    ret     =   avfilter_graph_create_filter( &buffersrcContext, buffersrc, "in", args.c_str(), nullptr, filterGraph );
    if( ret < 0 ) 
    {
        release();
        MYLOG( LOG::ERROR, "error" );
        return false;
    }

    if( avfilter_graph_create_filter( &buffersinkContext, buffersink, "out", nullptr, nullptr, filterGraph) < 0 )
    {
        //qDebug() << "Has Error: line =" << __LINE__;
        MYLOG( LOG::ERROR, "error" );
        release();
        return false;
    }

    output->name = av_strdup("in");
    output->next = nullptr;
    output->pad_idx = 0;
    output->filter_ctx = buffersrcContext;

    input->name = av_strdup("out");
    input->next = nullptr;
    input->pad_idx = 0;
    input->filter_ctx = buffersinkContext;

    ret     =   avfilter_graph_parse_ptr(filterGraph, filterDesc.c_str(), &input, &output, nullptr);
    if (ret < 0) 
    {
        //qDebug() << "Has Error: line =" << __LINE__;
        MYLOG( LOG::ERROR, "error" );
        release();
        return false;
    }

    char *str = avfilter_graph_dump( filterGraph, NULL );
    MYLOG( LOG::DEBUG, "options = %s", str );

    if (avfilter_graph_config(filterGraph, nullptr) < 0) 
    {
        //qDebug() << "Has Error: line =" << __LINE__;
        MYLOG( LOG::DEBUG, "error" );
        release();
        return false;
    }

    release();
    return true;
}




/*******************************************************************************
SubDecode::generate_subtitle_image()

�o�ӵ{���X�S����L, �O�q�����ƻs�L�Ӫ�, �Q��k����.
********************************************************************************/
void    SubDecode::generate_subtitle_image(  AVSubtitle &subtitle )
{
    int     i;

    for( i = 0; i < subtitle.num_rects; i++ )
    {
        AVSubtitleRect  *sub_rect   =   subtitle.rects[i];

        int     dst_linesize[4];
        uint8_t *dst_data[4];

        //
        av_image_alloc( dst_data, dst_linesize, sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA, 1 );

        SwsContext *swsContext  =   sws_getContext( sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
                                                    sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA,
                                                    SWS_BILINEAR, nullptr, nullptr, nullptr );

        sws_scale( swsContext, sub_rect->data, sub_rect->linesize, 0, sub_rect->h, dst_data, dst_linesize );
        sws_freeContext(swsContext);        

        sub_image  =   QImage(dst_data[0], sub_rect->w, sub_rect->h, QImage::Format_RGBA8888).copy();
        av_freep(&dst_data[0]);
    }
}




/*******************************************************************************
SubDecode::decode_subtitle()
********************************************************************************/
int    SubDecode::decode_subtitle( AVPacket* pkt )
{
    AVCodecContext  *dec    =   pkt == nullptr ? dec_map[cs_idx] : dec_map[pkt->stream_index];

    AVSubtitle  subtitle {0};

    int     got_sub     =   0;
    int     ret         =   avcodec_decode_subtitle2( dec, &subtitle, &got_sub, pkt );
    
    if( ret >= 0 && got_sub > 0 )
    {
        if( got_sub > 0 )
        {
            //MYLOG( LOG::DEBUG, "decode subtitle.");

            // �N��r���O�Ϥ��榡, �ݭn���͹������r������.
            if (subtitle.format == 0)
                generate_subtitle_image( subtitle );

            avsubtitle_free( &subtitle );
            return  SUCCESS;
        }
        else         
            return  SUCCESS;  // decode success, but no subtitle.
    }
    else
    {
        MYLOG( LOG::ERROR, "decode subtitle fail" );
        return  ERROR;
    }   
    

#if 0
    // �Ψӿ�X�T�������յ{���X
    qreal pts = pkt->pts * av_q2d(subStream->time_base);
    qreal duration = pkt->duration * av_q2d(subStream->time_base);

    // https://tsduck.io/doxy/namespacets.html
    // �i�H�� ts �M�󰵤�r�ഫ.
    const char *text = const_int8_ptr(pkt->data);
    MYLOG( LOG::DEBUG, "pts = %lf, duration = %lf, text = %s", pts, duration, pkt->data );
#endif

#if 0
    // �Ψӿ�X�T�������յ{���X
    AVSubtitleRect **rects = subtitle.rects;
    for (int i = 0; i < subtitle.num_rects; i++) 
    {
        AVSubtitleRect rect = *rects[i];
        if (rect.type == SUBTITLE_ASS)                 
            MYLOG( LOG::DEBUG, "ASS %s", rect.ass)                
        else if (rect.x == SUBTITLE_TEXT)                 
            MYLOG( LOG::DEBUG, "TEXT %s", rect.text)
    }
#endif
}







/*******************************************************************************
SubDecode::get_subtitle_image()
********************************************************************************/
QImage  SubDecode::get_subtitle_image()
{
    return  sub_image;
}





/*******************************************************************************
SubDecode::get_subtitle_image()

sws_ctx ���@��, �U�@�Ӷ��q��������
********************************************************************************/
int     SubDecode::generate_subtitle_image( AVFrame *video_frame, SwsContext *sws_ctx )
{
    //v_decoder.output_video_frame_info();
    //vdata   =   v_decoder.output_video_data();

    //AVFrame *frame = v_decoder.get_frame();
    //AVFrame *filter_frame = av_frame_alloc();

    int     ret     =   0;   

    ret     =   av_buffersrc_add_frame_flags( buffersrcContext, video_frame, AV_BUFFERSRC_FLAG_KEEP_REF );    
    if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "add frame flag fail." );
        return  -1;
    }

    //
    ret     =   av_buffersink_get_frame( buffersinkContext, frame );    
    if( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ) 
        return  0;  // �S���,���S���~.
    else if( ret < 0 )
    {
        MYLOG( LOG::ERROR, "get frame fail." );
        return  -1;
    }
    
    // generate image.
    // 1. Get frame and QImage to show 
    sub_image  =   QImage{ video_frame->width, video_frame->height, QImage::Format_RGB888 };  // ���ӦҼ{�����o�䪺code, �קK���ƪ�l��.
    
    // 2. Convert and write into image buffer  
    uint8_t *dst[]  =   { sub_image.bits() };
    int     linesizes[4];
    
    //SwsContext      *sws_ctx   =   v_decoder.get_sws_ctx();    
    
    av_image_fill_linesizes( linesizes, AV_PIX_FMT_RGB24, frame->width );
    sws_scale( sws_ctx, frame->data, (const int*)frame->linesize, 0, frame->height, dst, linesizes );
    
    av_frame_unref(frame);

    return 1;

    // �����ѦҪ��{���X�|�� loop �h������, �ˬd�@�U�O�_�u�����o��case. �ثe���ճ��O�@���u�Ǧ^�@�i��
    /*ret     =   av_buffersink_get_frame( buffersinkContext, frame );    
    if( ret >= 0 )
        MYLOG( LOG::ERROR, "get two data. fail" );*/


    //
    //vd.index        =   frame_count;
    //vd.frame        =   img;
    //vd.timestamp    =   get_timestamp();
    //vdata.frame = img;
    //dc->unref_frame();
    //av_frame_unref(filter_frame);
    //av_frame_free(&filter_frame);
    
    //video_queue.push(vdata);
    

}



















// ���դ���subtitle���{���X
#if 0
�����[�Wmutex lock, ���M�|�X���D
std::thread *thr = new std::thread( [this]() -> void {

    static int aaaa = 0;

    while(true) 
    {
        std::this_thread::sleep_for( std::chrono::seconds(1) );
        aaaa++;

        // �i�H�ʺA����
        std::string filterDesc;
        if( aaaa % 2 == 0 )
            filterDesc = "subtitles=filename='\\D\\:/code/test2.mkv':original_size=1920x1080:stream_index=0";  
        else
            filterDesc = "subtitles=filename='\\D\\:/code/test2.mkv':original_size=1920x1080:stream_index=1";  

        std::string args = "video_size=1920x1080:pix_fmt=64:time_base=1/1000:pixel_aspect=1/1";

        init_subtitle_filter( buffersrcContext, buffersinkContext,  args,  filterDesc);
    }
    } );
#endif



// ���դ���subtitle���{���X
#if 0
{
    static int aaaa = 0;

    // �i�H�ʺA����, ���n��mutex lock
    std::string filterDesc;
    if( aaaa % 2 == 0 )
        filterDesc = "subtitles=filename='\\D\\:/code/test2.mkv':original_size=1920x1080:stream_index=0";  
    else
        filterDesc = "subtitles=filename='\\D\\:/code/test2.mkv':original_size=1920x1080:stream_index=1";  

    std::string args = "video_size=1920x1080:pix_fmt=64:time_base=1/1000:pixel_aspect=1/1";

    init_subtitle_filter( buffersrcContext, buffersinkContext,  args,  filterDesc);

}
aaaa++;
#endif









