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

    void    open( EncodeSetting setting, AVCodecContext* v_ctx, AVCodecContext* a_ctx, InputOutput* IO );

    void    init( EncodeSetting setting ) override;
    void    write_end() override;
    void    end() override;

private:

    static constexpr    int     FFMPEG_OUTPUT_BUFFER_SIZE    =   4096;
    uint8_t         *output_buf     =   nullptr;
    AVIOContext     *io_ctx         =   nullptr;

};



int     io_write_data( void *opaque, uint8_t *buffer, int size );




#endif