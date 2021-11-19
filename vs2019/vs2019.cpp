#include <iostream>

#include "player.h"



/*
http://dranger.com/ffmpeg/tutorial05.html
http://dranger.com/ffmpeg/tutorial01.html

https://cpp.hotexamples.com/examples/-/-/avsubtitle_free/cpp-avsubtitle_free-function-examples.html



https://www.twblogs.net/a/5ef2639e26bc8c4a8eb3c99e
https://github.com/wang-bin/QtAV/blob/master/src/subtitle/SubtitleProcessorFFmpeg.cpp




ass_encode_frame
show_subtitle


decoder_decode_frame   ****
sub2video_update       *

try_decode_frame
init_subtitles


decode_frame

*/



int main()
{
    Player  player;

    player.init();
    player.play();
    player.end();

    return 0;
}



static int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub) 
{
    int ret = AVERROR(EAGAIN);

    for (;;) 
    {
        if (d->queue->serial == d->pkt_serial) 
        {
            do 
            {
                if (d->queue->abort_request)
                    return -1;

                switch (d->avctx->codec_type) 
                {
                    case AVMEDIA_TYPE_VIDEO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) 
                        {
                            if (decoder_reorder_pts == -1) 
                            {
                                frame->pts = frame->best_effort_timestamp;
                            } 
                            else if (!decoder_reorder_pts) 
                            {
                                frame->pts = frame->pkt_dts;
                            }
                        }
                        break;
                    case AVMEDIA_TYPE_AUDIO:
                        ret = avcodec_receive_frame(d->avctx, frame);
                        if (ret >= 0) 
                        {
                            AVRational tb = (AVRational){1, frame->sample_rate};
                            if (frame->pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
                            else if (d->next_pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                            if (frame->pts != AV_NOPTS_VALUE) 
                            {
                                d->next_pts = frame->pts + frame->nb_samples;
                                d->next_pts_tb = tb;
                            }
                        }
                        break;
                }

                if (ret == AVERROR_EOF) 
                {
                    d->finished = d->pkt_serial;
                    avcodec_flush_buffers(d->avctx);
                    return 0;
                }
                if (ret >= 0)
                    return 1;
            } while (ret != AVERROR(EAGAIN));
        }

        do {
            if (d->queue->nb_packets == 0)
                SDL_CondSignal(d->empty_queue_cond);
            
            if (d->packet_pending) 
            {
                d->packet_pending = 0;
            } 
            else 
            {
                int old_serial = d->pkt_serial;
                if (packet_queue_get(d->queue, d->pkt, 1, &d->pkt_serial) < 0)
                    return -1;
                if (old_serial != d->pkt_serial) {
                    avcodec_flush_buffers(d->avctx);
                    d->finished = 0;
                    d->next_pts = d->start_pts;
                    d->next_pts_tb = d->start_pts_tb;
                }
            }

            if (d->queue->serial == d->pkt_serial)
                break;

            av_packet_unref(d->pkt);
        } while (1);


        if (d->avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) 
        {
            int got_frame = 0;
            ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, d->pkt);
            if (ret < 0) 
            {
                ret = AVERROR(EAGAIN);
            } 
            else 
            {
                if (got_frame && !d->pkt->data) 
                {
                    d->packet_pending = 1;
                }
                ret = got_frame ? 0 : (d->pkt->data ? AVERROR(EAGAIN) : AVERROR_EOF);
            }
            av_packet_unref(d->pkt);
        } 
        else 
        {
            if (avcodec_send_packet(d->avctx, d->pkt) == AVERROR(EAGAIN)) 
            {
                av_log(d->avctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                d->packet_pending = 1;
            } 
            else 
            {
                av_packet_unref(d->pkt);
            }
        }
    }
}




static int get_video_frame(VideoState *is, AVFrame *frame)
{
    int got_picture;

    if ((got_picture = decoder_decode_frame(&is->viddec, frame, NULL)) < 0)
        return -1;

    if (got_picture) 
    {
        double dpts = NAN;

        if (frame->pts != AV_NOPTS_VALUE)
            dpts = av_q2d(is->video_st->time_base) * frame->pts;

        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

        if (framedrop>0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) 
        {
            if (frame->pts != AV_NOPTS_VALUE) 
            {
                double diff = dpts - get_master_clock(is);
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
                    diff - is->frame_last_filter_delay < 0 &&
                    is->viddec.pkt_serial == is->vidclk.serial &&
                    is->videoq.nb_packets) 
                {
                    is->frame_drops_early++;
                    av_frame_unref(frame);
                    got_picture = 0;
                }
            }
        }
    }

    return got_picture;
}




typedef struct Frame {
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;           /* presentation timestamp for the frame */
    double duration;      /* estimated duration of the frame */
    int64_t pos;          /* byte position of the frame in the input file */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
} Frame;



static int subtitle_thread(void *arg)
{
    VideoState *is = arg;
    Frame *sp;
    int got_subtitle;
    double pts;

    for (;;) 
    {
        if (!(sp = frame_queue_peek_writable(&is->subpq)))
            return 0;

        if ((got_subtitle = decoder_decode_frame(&is->subdec, NULL, &sp->sub)) < 0)
            break;

        pts = 0;

        if (got_subtitle && sp->sub.format == 0) 
        {
            if (sp->sub.pts != AV_NOPTS_VALUE)
                pts = sp->sub.pts / (double)AV_TIME_BASE;
            sp->pts = pts;
            sp->serial = is->subdec.pkt_serial;
            sp->width = is->subdec.avctx->width;
            sp->height = is->subdec.avctx->height;
            sp->uploaded = 0;

            /* now we can update the picture count */
            frame_queue_push(&is->subpq);
        } 
        else if (got_subtitle) 
        {
            avsubtitle_free(&sp->sub);
        }
    }
    return 0;
}



static void frame_queue_push(FrameQueue *f)
{
    if (++f->windex == f->max_size)
        f->windex = 0;
    SDL_LockMutex(f->mutex);
    f->size++;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}






///////////////////////////////////////////////////////////////////////


void ffProcessSubtitlePacket( AVPacket *pkt )
{
    //LOGI("NATIVE FFMPEG SUBTITLE - Decoding subtitle packet");

    int got = 0;

    avcodec_decode_subtitle2(ffSubtitleContext, &ffSubtitleFrame, &got, pkt);

    if ( got )
    {
        //LOGI("NATIVE FFMPEG SUBTITLE - Got subtitle frame");
        //LOGI("NATIVE FFMPEG SUBTITLE - Format = %d, Start = %d, End = %d, Rects = %d, PTS = %llu, AudioPTS = %llu, PacketPTS = %llu",
        //      ffSubtitleFrame.format, ffSubtitleFrame.start_display_time,
        //      ffSubtitleFrame.end_display_time, ffSubtitleFrame.num_rects,
        //      ffSubtitleFrame.pts, ffAudioGetPTS(), pkt->pts);

        // now add the subtitle data to the list ready

        for ( int s = 0; s < ffSubtitleFrame.num_rects; s++ )
        {
            ffSubtitle *sub = (ffSubtitle*)mmAlloc(sizeof(ffSubtitle)); //new ffSubtitle;

            if ( sub )
            {
                AVSubtitleRect *r = ffSubtitleFrame.rects[s];
                AVPicture *p = &r->pict;

                // set main data

                sub->startPTS   = pkt->pts + (uint64_t)ffSubtitleFrame.start_display_time;
                sub->endPTS     = pkt->pts + (uint64_t)ffSubtitleFrame.end_display_time * (uint64_t)500;
                sub->nb_colors  = r->nb_colors;
                sub->xpos       = r->x;
                sub->ypos       = r->y;
                sub->width      = r->w;
                sub->height     = r->h;

                // allocate space for CLUT and image all in one chunk

                sub->data       = mmAlloc(r->nb_colors * 4 + r->w * r->h); //new char[r->nb_colors * 4 + r->w * r->h];

                if ( sub->data )
                {
                    // copy the CLUT data

                    memcpy(sub->data, p->data[1], r->nb_colors * 4);

                    // copy the bitmap onto the end

                    memcpy(sub->data + r->nb_colors * 4, p->data[0], r->w * r->h);

                    // check for duplicate subtitles and remove them as this
                    // one replaces it with a new bitmap data

                    int pos = ffSubtitles.size();

                    while ( pos-- )
                    {
                        ffSubtitle *s = ffSubtitles[pos];
                        if ( s->xpos == sub->xpos &&
                            s->ypos == sub->ypos &&
                            s->width == sub->width &&
                            s->height == sub->height )
                        {
                            //delete s;
                            ffSubtitles.erase( ffSubtitles.begin() + pos );

                            //LOGI("NATIVE FFMPEG SUBTITLE - Removed old duplicate subtitle, size %d", ffSubtitles.size());
                        }
                    }

                    // append to subtitles list

                    ffSubtitles.push_back( sub );

                    char *dat;  // data pointer used for the CLUT table

                                //LOGI("NATIVE FFMPEG SUBTITLE - Added %d,%d - %d,%d, Queue %d, Length = %d",
                                //  r->x, r->y, r->w, r->h, ffSubtitles.size(), ffSubtitleFrame.end_display_time);

                                // convert the CLUT (RGB) to YUV values

                    dat = sub->data;

                    for ( int c = 0; c < r->nb_colors; c++ )
                    {
                        int r = dat[0];
                        int g = dat[1];
                        int b = dat[2];

                        int y = ( (  65 * r + 128 * g +  24 * b + 128) >> 8) +  16;
                        int u = ( ( -37 * r -  74 * g + 112 * b + 128) >> 8) + 128;
                        int v = ( ( 112 * r -  93 * g -  18 * b + 128) >> 8) + 128;

                        *dat++ = (char)y;
                        *dat++ = (char)u;
                        *dat++ = (char)v;
                        dat++;  // skip the alpha channel
                    }
                }
                else
                {
                    //delete sub;
                    sub = 0;
                    LOGI("NATIVE FFMPEG SUBTITLE - Memory allocation error CLUT and BITMAP");
                }
            }
            else
            {
                LOGI("NATIVE FFMPEG SUBTITLE - Memory allocation error ffSubtitle struct");
                mmGarbageCollect();
                ffSubtitles.clear();
            }
        }
    }
}

void ffSubtitleRenderCheck(int bpos)
{
    if ( ffSubtitleID == -1 || !usingSubtitles )
    {
        // empty the list in case of memory leaks

        ffSubtitles.clear();
        mmGarbageCollect();
        return;
    }

    uint64_t audioPTS = ffAudioGetPTS();
    int pos = 0;

    // draw the subtitle list to the YUV frames

    char *yframe = ffVideoBuffers[bpos].yFrame;
    char *uframe = ffVideoBuffers[bpos].uFrame;
    char *vframe = ffVideoBuffers[bpos].vFrame;

    int ywidth = fv.frameActualWidth;   // actual width with padding
    int uvwidth = fv.frameAWidthHalf;   // and for uv frames

    while ( pos < ffSubtitles.size() )
    {
        ffSubtitle *sub = ffSubtitles[pos];

        if ( sub->startPTS >= audioPTS ) // okay to draw this one?
        {
            //LOGI("NATIVE FFMPEG SUBTITLE - Rendering subtitle bitmap %d", pos);

            char *clut = sub->data;     // colour table
            char *dat = clut + sub->nb_colors * 4; // start of bitmap data

            int w = sub->width;
            int h = sub->height;
            int x = sub->xpos;
            int y = sub->ypos;

            for ( int xpos = 0; xpos < w; xpos++ )
            {
                for ( int ypos = 0; ypos < h; ypos++ )
                {
                    // get colour for pixel
                    char bcol = dat[ypos * w + xpos];

                    if ( bcol != 0 )    // ignore 0 pixels
                    {
                        char cluty = clut[bcol * 4 + 0];    // get colours from CLUT
                        char clutu = clut[bcol * 4 + 1];
                        char clutv = clut[bcol * 4 + 2];

                        // draw to Y frame

                        int newx = x + xpos;
                        int newy = y + ypos;

                        yframe[newy * ywidth + newx] = cluty;

                        // draw to uv frames if we have a quarter pixel only

                        if ( ( newy & 1 ) && ( newx & 1 ) )
                        {
                            uframe[(newy >> 1) * uvwidth + (newx >> 1)] = clutu;
                            vframe[(newy >> 1) * uvwidth + (newx >> 1)] = clutv;
                        }
                    }
                }
            }
        }

        pos++;
    }

    // Last thing is to erase timed out subtitles

    pos = ffSubtitles.size();

    while ( pos-- )
    {
        ffSubtitle *sub = ffSubtitles[pos];

        if ( sub->endPTS < audioPTS )
        {
            //delete sub;
            ffSubtitles.erase( ffSubtitles.begin() + pos );

            //LOGI("NATIVE FFMPEG SUBTITLE - Removed timed out subtitle");
        }
    }

    if ( ffSubtitles.size() == 0 )
    {
        // garbage collect the custom memory pool

        mmGarbageCollect();
    }

    //LOGI("NATIVE FFMPEG SUBTITLE - Size of subtitle list = %d", ffSubtitles.size());
}







bool SubtitleProcessorFFmpeg::processSubtitle()
{
    m_frames.clear();
    int ss = m_reader.subtitleStream();
    if (ss < 0) 
    {
        qWarning("no subtitle stream found");
        return false;
    }


    codec_ctx = m_reader.subtitleCodecContext();
    AVCodec *dec = avcodec_find_decoder(codec_ctx->codec_id);

    const AVCodecDescriptor *dec_desc = avcodec_descriptor_get(codec_ctx->codec_id);
    if (!dec) 
    {
        if (dec_desc)
            qWarning("Failed to find subtitle codec %s", dec_desc->name);
        else
            qWarning("Failed to find subtitle codec %d", codec_ctx->codec_id);
        return false;
    }

    qDebug("found subtitle decoder '%s'", dec_desc->name);


    // AV_CODEC_PROP_TEXT_SUB: ffmpeg >= 2.0
#ifdef AV_CODEC_PROP_TEXT_SUB
    if (dec_desc && !(dec_desc->props & AV_CODEC_PROP_TEXT_SUB)) {
        qWarning("Only text based subtitles are currently supported");
        return false;
    }
#endif



    AVDictionary *codec_opts = NULL;
    int ret = avcodec_open2(codec_ctx, dec, &codec_opts);
    if (ret < 0) 
    {
        qWarning("open subtitle codec error: %s", av_err2str(ret));
        av_dict_free(&codec_opts);
        return false;
    }


    while (!m_reader.atEnd()) 
    {
        if (!m_reader.readFrame()) { // eof or other errors
            continue;
        }
        if (m_reader.stream() != ss)
            continue;
        const Packet pkt = m_reader.packet();
        if (!pkt.isValid())
            continue;
        SubtitleFrame frame = processLine(pkt.data, pkt.pts, pkt.duration);
        if (frame.isValid())
            m_frames.append(frame);
    }
    avcodec_close(codec_ctx);
    codec_ctx = 0;
    return true;
}





m_fps = videoStream->avg_frame_rate.num / videoStream->avg_frame_rate.den;
m_width = videoCodecContext->width;
m_height = videoCodecContext->height;

//初始化filter相關
AVRational time_base = videoStream->time_base;
QString args = QString::asprintf("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
    m_width, m_height, videoCodecContext->pix_fmt, time_base.num, time_base.den,
    videoCodecContext->sample_aspect_ratio.num, videoCodecContext->sample_aspect_ratio.den);
qDebug() << "Video Args: " << args;

AVFilterContext *buffersrcContext = nullptr;
AVFilterContext *buffersinkContext = nullptr;
bool subtitleOpened = false;

//如果有字幕流
if (subCodecContext) 
{
    //字幕流直接用視頻名即可
    QString subtitleFilename = m_filename;
    subtitleFilename.replace('/', "\\\\");
    subtitleFilename.insert(subtitleFilename.indexOf(":\\"), char('\\'));
    QString filterDesc = QString("subtitles=filename='%1':original_size=%2x%3")
        .arg(subtitleFilename).arg(m_width).arg(m_height);
    qDebug() << "Filter Description:" << filterDesc.toStdString().c_str();



    subtitleOpened = init_subtitle_filter(buffersrcContext, buffersinkContext, args, filterDesc);



    if (!subtitleOpened) {
        qDebug() << "字幕打開失敗!";
    }
} 
else 
{
    //沒有字幕流時，在同目錄下尋找字幕文件
    //字幕相關，使用subtitles，目前測試的是ass，但srt, ssa, ass, lrc都行，改後綴名即可
    int suffixLength = QFileInfo(m_filename).suffix().length();
    QString subtitleFilename = m_filename.mid(0, m_filename.length() - suffixLength - 1) + ".ass";
    if (QFile::exists(subtitleFilename)) {
        //初始化subtitle filter
        //絕對路徑必須轉成D\:\\xxx\\test.ass這種形式, 記住，是[D\:\\]這種形式
        //toNativeSeparator()無用，因爲只是 / -> \ 的轉換
        subtitleFilename.replace('/', "\\\\");
        subtitleFilename.insert(subtitleFilename.indexOf(":\\"), char('\\'));
        QString filterDesc = QString("subtitles=filename='%1':original_size=%2x%3")
            .arg(subtitleFilename).arg(m_width).arg(m_height);
        qDebug() << "Filter Description:" << filterDesc.toStdString().c_str();
        subtitleOpened = init_subtitle_filter(buffersrcContext, buffersinkContext, args, filterDesc);
        if (!subtitleOpened) {
            qDebug() << "字幕打開失敗!";
        }
    }
}





SubtitleFrame subFrame;

//讀取下一幀
while (m_runnable && av_read_frame(formatContext, packet) >= 0) 
{
    if (packet->stream_index == videoIndex) 
    {
        //發送給解碼器
        int ret = avcodec_send_packet(videoCodecContext, packet);

        while (ret >= 0) 
        {
            //從解碼器接收解碼後的幀
            ret = avcodec_receive_frame(videoCodecContext, frame);

            frame->pts = frame->best_effort_timestamp;

            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            else if (ret < 0) goto Run_End;

            //如果字幕成功打開，則輸出使用subtitle filter過濾後的圖像
            if (subtitleOpened) 
            {
                if (av_buffersrc_add_frame_flags(buffersrcContext, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                    break;

                while (true) 
                {
                    ret = av_buffersink_get_frame(buffersinkContext, filter_frame);

                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                    else if (ret < 0) goto Run_End;

                    QImage videoImage = convert_image(filter_frame);
                    m_frameQueue.enqueue(videoImage);

                    av_frame_unref(filter_frame);
                }
            } 
            else 
            {
                //未打開字幕過濾器或無字幕
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                else if (ret < 0) goto Run_End;

                QImage videoImage = convert_image(frame);
                //如果需要顯示字幕，就將字幕覆蓋上去
                if (frame->pts >= subFrame.pts && frame->pts <= (subFrame.pts + subFrame.duration)) 
                {
                    videoImage = overlay_subtitle(videoImage, subFrame.image);
                }
                m_frameQueue.enqueue(videoImage);
            }
            av_frame_unref(frame);
        }
    } 
    else if (packet->stream_index == subIndex) 
    {
        AVSubtitle subtitle;
        int got_frame;
        int ret = avcodec_decode_subtitle2(subCodecContext, &subtitle, &got_frame, packet);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        else if (ret < 0) goto Run_End;

        if (got_frame > 0) 
        {
            //如果是圖像字幕，即sub + idx
            //實際上，只需要處理這種即可
            if (subtitle.format == 0) 
            {
                for (size_t i = 0; i < subtitle.num_rects; i++) 
                {
                    AVSubtitleRect *sub_rect = subtitle.rects[i];

                    int dst_linesize[4];
                    uint8_t *dst_data[4];
                    //注意，這裏是RGBA格式，需要Alpha
                    av_image_alloc(dst_data, dst_linesize, sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA, 1);
                    SwsContext *swsContext = sws_getContext(sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
                        sub_rect->w, sub_rect->h, AV_PIX_FMT_RGBA,
                        SWS_BILINEAR, nullptr, nullptr, nullptr);
                    sws_scale(swsContext, sub_rect->data, sub_rect->linesize, 0, sub_rect->h, dst_data, dst_linesize);
                    sws_freeContext(swsContext);
                    //這裏也使用RGBA
                    QImage image = QImage(dst_data[0], sub_rect->w, sub_rect->h, QImage::Format_RGBA8888).copy();
                    av_freep(&dst_data[0]);

                    //subFrame存儲當前的字幕
                    //只有圖像字幕纔有start_display_time和start_display_time
                    subFrame.pts = packet->pts;
                    subFrame.duration = subtitle.end_display_time - subtitle.start_display_time;
                    subFrame.image = image;
                }
            } 
            else 
            {
                //如果是文本格式字幕:srt, ssa, ass, lrc
                //可以直接輸出文本，實際上已經添加到過濾器中
                qreal pts = packet->pts * av_q2d(subStream->time_base);
                qreal duration = packet->duration * av_q2d(subStream->time_base);
                const char *text = const_int8_ptr(packet->data);
                qDebug() << "[PTS: " << pts << "]" << endl
                    << "[Duration: " << duration << "]" << endl
                    << "[Text: " << text << "]" << endl;
            }
        }
    }
