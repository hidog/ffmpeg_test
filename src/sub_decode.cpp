#include "sub_decode.h"
#include "tool.h"

extern "C" {

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libswresample/swresample.h>

#include <libavfilter/buffersrc.h>  // use for subtitle
#include <libavfilter/buffersink.h>

} // end extern "C"






/*******************************************************************************
SubDecode::SubDecode()
********************************************************************************/
SubDecode::SubDecode()
    :   Decode()
{
    AVMediaType   type  =   AVMEDIA_TYPE_SUBTITLE; 
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
int     SubDecode::open_codec_context( int stream_index, AVFormatContext *fmt_ctx )
{
    if( stream_index < 0 )
    {
        MYLOG( LOG::INFO, "no sub stream" );
        return  SUCCESS;
    }

    Decode::open_codec_context( stream_index, fmt_ctx, type );
    //dec_ctx->thread_count = 4;
    return  SUCCESS;
}





/*******************************************************************************
SubDecode::init()
********************************************************************************/
int     SubDecode::init()
{
    return  SUCCESS;
}





/*******************************************************************************
SubDecode::end()
********************************************************************************/
int     SubDecode::end()
{
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
bool SubDecode::init_subtitle_filter( std::string args, std::string filterDesc)
{
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *output = avfilter_inout_alloc();
    AVFilterInOut *input = avfilter_inout_alloc();
    AVFilterGraph *filterGraph = avfilter_graph_alloc();
    //filterGraph = avfilter_graph_alloc();

    // lambda operator, �ٲ��F�ǤJ�ѼƬA��. 
    auto release = [&output, &input] 
    {
        avfilter_inout_free(&output);
        avfilter_inout_free(&input);
    };

    if (!output || !input || !filterGraph) 
    {
        release();
        return false;
    }

    // Crear filtro de entrada, necesita arg
    if (avfilter_graph_create_filter(&buffersrcContext, buffersrc, "in", args.c_str(), nullptr, filterGraph) < 0) 
    {
        //qDebug() << "Has Error: line =" << __LINE__;
        MYLOG( LOG::ERROR, "error" );
        release();
        return false;
    }

    if (avfilter_graph_create_filter(&buffersinkContext, buffersink, "out", nullptr, nullptr, filterGraph) < 0) 
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

    int ret = avfilter_graph_parse_ptr(filterGraph, filterDesc.c_str(), &input, &output, nullptr);
    if (ret < 0) 
    {
        //qDebug() << "Has Error: line =" << __LINE__;
        MYLOG( LOG::DEBUG, "error" );
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
