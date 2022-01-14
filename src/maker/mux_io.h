#ifndef MUX_IO_H
#define MUX_IO_H


#include "mux.h"
#include "../IO/input_output.h"


struct AVIOContext;



class MuxIO : public Mux
{
public:

    MuxIO();
    ~MuxIO();

    MuxIO( const MuxIO& ) = delete;
    MuxIO( MuxIO&& ) = delete;

    MuxIO& operator = ( const MuxIO& ) = delete;
    MuxIO& operator = ( MuxIO&& ) = delete;

    void    open( EncodeSetting setting, AVCodecContext* v_ctx, AVCodecContext* a_ctx, AVCodecContext* s_ctx ) override;
    void    init( EncodeSetting setting ) override;
    void    write_end() override;
    bool    is_connect() override;
    void    end() override;

    void    init_IO( EncodeSetting setting );

private:

    static constexpr    int     FFMPEG_OUTPUT_BUFFER_SIZE    =   4096;
    uint8_t     output_buf[FFMPEG_OUTPUT_BUFFER_SIZE];

    AVIOContext*    io_ctx  =   nullptr;
    InputOutput*    IO      =   nullptr;

};



#endif