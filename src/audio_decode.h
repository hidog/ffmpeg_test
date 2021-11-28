#ifndef AUDIO_DECODE_H
#define AUDIO_DECODE_H

#include "decode.h"


struct SwrContext;
struct AVCodec;
struct AVCodecContext;
enum   AVSampleFormat;


// 試著要轉sample rate但沒成功,有機會再試試看.
// 要轉 sample rate 可以參考 ffplay, 需要將buffer size做 sample rate的轉換.  (例如 48000/44100 )



class DLL_API AudioDecode : public Decode
{
public:
    AudioDecode();
    ~AudioDecode();

    AudioDecode( const AudioDecode& ) = delete;
    AudioDecode( AudioDecode&& ) = delete;

    AudioDecode& operator = ( const AudioDecode& ) = delete;
    AudioDecode& operator = ( AudioDecode&& ) = delete;
    
    int     open_codec_context( AVFormatContext *fmt_ctx ) override;
    void    output_decode_info( AVCodec *dec, AVCodecContext *dec_ctx ) override;
    
    int     init() override;
    int     end() override;

    int     get_audio_channel();
    int     get_audio_sample_rate();
    
    void        output_audio_frame_info();    
    AudioData   output_audio_data();
    int64_t     get_timestamp();



    int     audio_info(); // 目前無作用 未來考慮移除

private:

    int         sample_rate     =   0;
    uint64_t    channel_layout  =   0;  // 多聲道的時候研究一下

    AVSampleFormat  sample_fmt;

    AVMediaType     type;    
    SwrContext      *swr_ctx    =   nullptr;   // use for chagne audio data to play.

    //AVCodecID   a_codec_id;

};




#endif