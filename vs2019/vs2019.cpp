#include <iostream>

#include "player/player.h"


/*
https://github.com/leandromoreira/ffmpeg-libav-tutorial     教學
https://kkc.github.io/2019/01/12/ffmpeg-libav-decode-note/

http://dranger.com/ffmpeg/tutorial05.html  // 教學
http://dranger.com/ffmpeg/tutorial01.html

https://github.com/wang-bin/QtAV   // 其他人用QT寫的播放器

https://cloud.tencent.com/developer/article/1357993  // 可參考的教學
https://www.itread01.com/content/1546569392.html     // ffmpeg 參數

https://www.programmersought.com/article/85494009767/   ffmpeg教學
https://github.com/mengps/FFmpeg-Learn

https://www.cnblogs.com/linuxAndMcu/p/14706012.html

https://www.jianshu.com/p/fd43c1c82945
https://www.jianshu.com/p/bf5e54f553a4

https://www.itread01.com/content/1545479823.html
*/


#include "../ffmpeg_example/muxing.h"
#include "maker/maker.h"


extern "C" {
#include "../ffmpeg_example/resampling_audio.h"
}




int main()
{
#if 0
    //test_aac();

    //resample_audio();
    //muxing();

    AudioEncode ae;
    ae.list_sample_format(AV_CODEC_ID_VORBIS);

#elif 0

    EncodeSetting   setting;
    //setting.filename    =   "J:\\test2.mkv";
    //setting.extension   =   "matroska";
    //setting.filename    =   "J:\\test2.mp4";
    //setting.extension   =   "mp4";
    setting.filename    =   "J:\\test2.avi";  // rmvb 是 variable bitrate. 目前還無法使用
    setting.extension   =   "avi";

    VideoEncodeSetting  v_setting;
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
    v_setting.gop_size      =   30;
    v_setting.max_b_frames  =   15; 
    //v_setting.gop_size      =   12;
    //v_setting.max_b_frames  =   0; 


    v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;

    v_setting.src_width     =   1920;
    v_setting.src_height    =   1080;
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGRA;    // for QImage
    //v_setting.src_pix_fmt   =   AV_PIX_FMT_BGR24;    // for openCV


    AudioEncodeSetting  a_setting;
    a_setting.code_id     =   AV_CODEC_ID_MP3;
    //a_setting.code_id       =   AV_CODEC_ID_AAC;
    //a_setting.code_id       =   AV_CODEC_ID_AC3;
    //a_setting.code_id     =   AV_CODEC_ID_MP2;
    //a_setting.code_id       =   AV_CODEC_ID_VORBIS;

    a_setting.bit_rate          =   320000;
    a_setting.sample_rate       =   48000;
    a_setting.channel_layout    =   3; // AV_CH_LAYOUT_STEREO = 3;



    Maker maker;

    maker.init( setting, v_setting, a_setting );
    maker.work();
    maker.end();

#else

    Player  player;  

    player.set_input_file("D:/code/test2.mkv");
    player.set_sub_file("D:/code/test2.mkv");

    player.init();
    player.play();
    player.end();
#endif

    return 0;
}




static void do_subtitle_out(OutputFile *of,
                            OutputStream *ost,
                            AVSubtitle *sub)
{
    int subtitle_out_max_size = 1024 * 1024;
    int subtitle_out_size, nb, i;
    AVCodecContext *enc;
    AVPacket pkt;
    int64_t pts;

    if (sub->pts == AV_NOPTS_VALUE) {
        av_log(NULL, AV_LOG_ERROR, "Subtitle packets must have a pts\n");
        if (exit_on_error)
             exit_program(1);
        return;
    }

    enc = ost->enc_ctx;

    if (!subtitle_out) {
        subtitle_out = av_malloc(subtitle_out_max_size);
        if (!subtitle_out) {
             av_log(NULL, AV_LOG_FATAL, "Failed to allocate subtitle_out\n");
             exit_program(1);
         }
    }

    /* Note: DVB subtitle need one packet to draw them and one other
    998        packet to clear them */
    /* XXX: signal it in the codec context ? */
     if (enc->codec_id == AV_CODEC_ID_DVB_SUBTITLE)
         nb = 2;
 else
         nb = 1;

     /* shift timestamp to honor -ss and make check_recording_time() work with -t */
     pts = sub->pts;
 if (output_files[ost->file_index]->start_time != AV_NOPTS_VALUE)
         pts -= output_files[ost->file_index]->start_time;
 for (i = 0; i < nb; i++) {
         unsigned save_num_rects = sub->num_rects;
 
         ost->sync_opts = av_rescale_q(pts, AV_TIME_BASE_Q, enc->time_base);
     if (!check_recording_time(ost))
             return;

         sub->pts = pts;
     // start_display_time is required to be 0
         sub->pts               += av_rescale_q(sub->start_display_time, (AVRational){ 1, 1000 }, AV_TIME_BASE_Q);
     sub->end_display_time  -= sub->start_display_time;
     sub->start_display_time = 0;
     if (i == 1)
             sub->num_rects = 0;

         ost->frames_encoded++;

         subtitle_out_size = avcodec_encode_subtitle(enc, subtitle_out,
                                                    subtitle_out_max_size, sub);
if (i == 1)
        sub->num_rects = save_num_rects;
if (subtitle_out_size < 0) {
        av_log(NULL, AV_LOG_FATAL, "Subtitle encoding failed\n");
        exit_program(1);
    }

    av_init_packet(&pkt);
pkt.data = subtitle_out;
pkt.size = subtitle_out_size;
pkt.pts  = av_rescale_q(sub->pts, AV_TIME_BASE_Q, ost->mux_timebase);
pkt.duration = av_rescale_q(sub->end_display_time, (AVRational){ 1, 1000 }, ost->mux_timebase);
if (enc->codec_id == AV_CODEC_ID_DVB_SUBTITLE) {
        /* XXX: the pts correction is handled here. Maybe handling
        1042                it in the codec would be better */
            if (i == 0)
                pkt.pts += av_rescale_q(sub->start_display_time, (AVRational){ 1, 1000 }, ost->mux_timebase);
        else
1046                 pkt.pts += av_rescale_q(sub->end_display_time, (AVRational){ 1, 1000 }, ost->mux_timebase);
         }
         pkt.dts = pkt.pts;
         output_packet(of, &pkt, ost, 0);
     }
 }

void saveSubtitle( AVFormatContext*context, Stream stream )
{
    stringstream outfile;
    outfile << "/tmp/subtitle_"<< stream.index << ".srt";
    string filename = outfile.str();

    AVStream *avstream = context->streams[stream.index];
    AVCodec *codec = avcodec_find_decoder( avstream->codec->codec_id);

    int result = avcodec_open2( avstream->codec, codec,NULL );
    checkResult( result == 0, "Error opening codec");
    cerr << "found codec: " << codec <<", open result= " << result << endl;
    
    AVOutputFormat *outFormat = av_guess_format( NULL, filename.c_str(),NULL );
    checkResult( outFormat != NULL, "Error finding format" );
    cerr << "Found output format: " << outFormat->name<< " (" << outFormat->long_name<< ")" << endl;
    
    AVFormatContext *outFormatContext;
    avformat_alloc_output_context2( &outFormatContext, NULL, NULL, filename.c_str());

    AVCodec *encoder = avcodec_find_encoder( outFormat->subtitle_codec);
    checkResult( encoder != NULL, "Error finding encoder" );
    cerr << "Found encoder: " << encoder->name<< endl;
    
    AVStream *outStream = avformat_new_stream( outFormatContext, encoder );
    checkResult( outStream != NULL, "Error allocating out stream" );
    
    AVCodecContext *c = outStream->codec;
    result = avcodec_get_context_defaults3( c, encoder);
    checkResult( result == 0, "error on get context default");
    cerr << "outstream codec: " << outStream->codec<< endl;
    cerr << "Opened stream " << outStream->id<< ", codec=" << outStream->codec->codec_id<< endl;
    result = avio_open(&outFormatContext->pb, filename.c_str(), AVIO_FLAG_WRITE);
    checkResult( result == 0, "Error opening out file");
    cerr << "out file opened correctly" << endl;
    result = avformat_write_header( outFormatContext,NULL );
    checkResult(result==0,"Error writing header");
    cerr << "header wrote correctly" << endl;
    result = 0;
    
    AVPacket pkt;
    av_init_packet( &pkt);
    pkt.data = NULL;
    pkt.size = 0;
    cerr << "srt codec id: " << AV_CODEC_ID_SUBRIP << endl;
    while( av_read_frame( context,&pkt ) >=0 )
    {
        if(pkt.stream_index!= stream.index)
            continue;
        int gotSubtitle =0;
        AVSubtitle subtitle;
        result = avcodec_decode_subtitle2( avstream->codec,&subtitle, &gotSubtitle, &pkt );
        uint64_t bufferSize = 1024 * 1024 ;
        uint8_t *buffer= new uint8_t[bufferSize];
        memset(buffer,0, bufferSize);
        if( result>= 0 )
        {
            result = avcodec_encode_subtitle( outStream->codec, buffer, bufferSize, &subtitle );
            cerr << "Encode subtitle result: " << result << endl;
        }
        cerr << "Encoded subtitle: " << buffer << endl;
        delete [] buffer;
    }
}

