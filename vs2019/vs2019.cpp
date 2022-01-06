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

https://libav.org/avconv.html#Subtitle-options_003a

https://en.wikibooks.org/wiki/FFMPEG_An_Intermediate_Guide/subtitle_options#Set_Subtitles_Character_Encoding_Conversion

https://www.itread01.com/content/1544555544.html







https://github.com/FFmpeg/FFmpeg/blob/master/libavfilter/vf_subtitles.c

static const AVOption subtitles_options[] = {
COMMON_OPTIONS
{"charenc",      "set input character encoding", OFFSET(charenc),      AV_OPT_TYPE_STRING, {.str = NULL}, 0, 0, FLAGS},
{"stream_index", "set stream index",             OFFSET(stream_index), AV_OPT_TYPE_INT,    { .i64 = -1 }, -1,       INT_MAX,  FLAGS},
{"si",           "set stream index",             OFFSET(stream_index), AV_OPT_TYPE_INT,    { .i64 = -1 }, -1,       INT_MAX,  FLAGS},
{"force_style",  "force subtitle style",         OFFSET(force_style),  AV_OPT_TYPE_STRING, {.str = NULL}, 0, 0, FLAGS},
{NULL},
};



-metadata:s:s:1 language=English 
-metadata:s:s:2 language=Dansk

ffmpeg -i input.mkv -map 0 -c copy -c:s mov_text -metadata:s:s:0 language=eng -metadata:s:s:1 language=ipk output.mp4


https://srtlab.github.io/srt-cookbook/apps/ffmpeg/
ffmpeg -f lavfi -re -i smptebars=duration=60:size=1280x720:rate=30 -f lavfi -re \
-i sine=frequency=1000:duration=60:sample_rate=44100 -pix_fmt yuv420p \
-c:v libx264 -b:v 1000k -g 30 -keyint_min 120 -profile:v baseline \
-preset veryfast -f mpegts "srt://127.0.0.1:4200?pkt_size=1316"




底下這個命令可以保持font size. 但需要研究
ffmpeg -i abc.mkv -i abc.srt -map 0:0 -map 0:1 -map 1:0 -c:s copy output.mkv



ffmpeg -h ass




https://www.jianshu.com/p/f33910818a1c



https://github.com/nldzsz/ffmpeg-demo/blob/master/cppsrc/Subtitles.cpp



https://gist.github.com/TBNolan/a887c5d069425119dd41461b779aa75b



https://blog.csdn.net/qq_21743659/article/details/109305411


http://www.famous1993.com.tw/tech/tech545.html
*/


/*
失敗
ffmpeg -i mysubs.srt -filter "subtitles=mysubs.srt:force_style='Fontname=Helvetica,Fontsize=16,PrimaryColour=&H23EEF1&,BackColour=&H000000&,BorderStyle=3,Outline=3,Shadow=0,Alignment=2,MarginL=10,MarginR=10,MarginV=10,Encoding=0'" mysubs.ass

ffmpeg -i mysubs.srt -filter 
"subtitles=mysubs.srt:force_style='
Fontname=Helvetica,
Fontsize=16,
PrimaryColour=&H23EEF1&,
BackColour=&H000000&,
BorderStyle=3,
Outline=3,S
hadow=0,
Alignment=2,
MarginL=10,
MarginR=10,
MarginV=10,
Encoding=0'" 
mysubs.ass
*/


#include "../ffmpeg_example/muxing.h"
#include "maker/maker.h"





int main()
{
    extract_subtitle_frome_file();
    printf("test");

#if 0
    muxing();
#elif 1

    EncodeSetting   setting;    
    // rmvb 是 variable bitrate. 目前還無法使用
    //setting.filename    =   "J:\\output.mkv";
    //setting.extension   =   "matroska";
    setting.filename    =   "J:\\output.mp4";
    setting.extension   =   "mp4";
    //setting.filename    =   "J:\\test2.avi"; 
    //setting.extension   =   "avi";

    setting.has_subtitle    =   true;



    VideoEncodeSetting  v_setting;
    //v_setting.code_id   =   AV_CODEC_ID_H264;
    v_setting.code_id   =   AV_CODEC_ID_H265;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG1VIDEO;
    //v_setting.code_id   =   AV_CODEC_ID_MPEG2VIDEO;

    v_setting.width     =   1920;
    v_setting.height    =   1080;

    v_setting.time_base.num     =   1001;
    v_setting.time_base.den     =   24000;

    /*
        b frame not support on rm
    */
    v_setting.gop_size      =   30;
    v_setting.max_b_frames  =   15; 
    //v_setting.gop_size      =   120;
    //v_setting.max_b_frames  =   80;


    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P;
    //v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P10LE;
    v_setting.pix_fmt   =   AV_PIX_FMT_YUV420P12LE;

    v_setting.src_width     =   1920;
    v_setting.src_height    =   1080;
    v_setting.src_pix_fmt   =   AV_PIX_FMT_BGRA;    // for QImage
    //v_setting.src_pix_fmt   =   AV_PIX_FMT_BGR24;    // for openCV


    AudioEncodeSetting  a_setting;
    //a_setting.code_id     =   AV_CODEC_ID_MP3;
    //a_setting.code_id       =   AV_CODEC_ID_AAC;
    a_setting.code_id       =   AV_CODEC_ID_AC3;
    //a_setting.code_id     =   AV_CODEC_ID_MP2;
    //a_setting.code_id       =   AV_CODEC_ID_VORBIS;
    //a_setting.code_id       =   AV_CODEC_ID_FLAC;

    a_setting.bit_rate          =   320000;
    a_setting.sample_rate       =   48000;
    a_setting.channel_layout    =   3; // AV_CH_LAYOUT_STEREO = 3;




    SubEncodeSetting   s_setting;
    //s_setting.code_id       =   AV_CODEC_ID_ASS;
    //s_setting.code_id       =   AV_CODEC_ID_SUBRIP;
    s_setting.code_id       =   AV_CODEC_ID_MOV_TEXT;
    s_setting.subtitle_file =   "J:\\test.ass";
    s_setting.subtitle_ext  =   "ass";


    Maker   maker;

    maker.init( setting, v_setting, a_setting, s_setting );
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









#if 0
    void FFmpeg::do_subtitle_out(OutputFile* of, OutputStream* ost, AVSubtitle* sub)
    {
        int subtitle_out_max_size = 1024 * 1024;
        int subtitle_out_size, nb, i;
        AVCodecContext* enc;
        AVPacket pkt;
        int64_t pts;

        if (sub->pts == AV_NOPTS_VALUE) {
            av_log(NULL, AV_LOG_ERROR, "Subtitle packets must have a pts\n");
            if (exit_on_error)
                _cmdUtils.exit_program(1);
            return;
        }

        enc = ost->enc_ctx;

        if (!subtitle_out) {
            subtitle_out = static_cast<uint8_t*>(av_malloc(subtitle_out_max_size));
            if (!subtitle_out) {
                av_log(NULL, AV_LOG_FATAL, "Failed to allocate subtitle_out\n");
                _cmdUtils.exit_program(1);
            }
        }

        /* Note: DVB subtitle need one packet to draw them and one other
           packet to clear them */
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

            ost->sync_opts = av_rescale_q(pts, AVRational{1, AV_TIME_BASE}, enc->time_base);
            if (!check_recording_time(ost))
                return;

            sub->pts = pts;
            // start_display_time is required to be 0
            sub->pts += av_rescale_q(sub->start_display_time, AVRational { 1, 1000 }, AVRational{1, AV_TIME_BASE});
            sub->end_display_time -= sub->start_display_time;
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
                _cmdUtils.exit_program(1);
            }

            av_init_packet(&pkt);
            pkt.data = subtitle_out;
            pkt.size = subtitle_out_size;
            pkt.pts = av_rescale_q(sub->pts, AVRational{1, AV_TIME_BASE}, ost->mux_timebase);
            pkt.duration = av_rescale_q(sub->end_display_time, AVRational { 1, 1000 }, ost->mux_timebase);
            if (enc->codec_id == AV_CODEC_ID_DVB_SUBTITLE) {
                /* XXX: the pts correction is handled here. Maybe handling
                   it in the codec would be better */
                if (i == 0)
                    pkt.pts += av_rescale_q(sub->start_display_time, AVRational { 1, 1000 }, ost->mux_timebase);
                else
                    pkt.pts += av_rescale_q(sub->end_display_time, AVRational { 1, 1000 }, ost->mux_timebase);
            }
            pkt.dts = pkt.pts;
            output_packet(of, &pkt, ost, 0);
        }
    }
#endif