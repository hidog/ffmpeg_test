#ifndef DEMUX_IO_H
#define DEMUX_IO_H

#include "demux.h"
#include "play_def.h"


class InputOutput;



class DemuxIO : public Demux 
{
public:
    DemuxIO( DecodeSetting _st );
    ~DemuxIO();

    DemuxIO( const DemuxIO& ) = delete;
    DemuxIO( DemuxIO&& ) = delete;

    DemuxIO& operator = ( const DemuxIO& ) = delete;
    DemuxIO& operator = ( DemuxIO&& ) = delete;

    int     open_input() override;
    int     init() override;    
    int     end() override;

private:

    InputOutput     *IO     =   nullptr;
    DecodeSetting   setting;

    static constexpr    int     FFMPEG_INPUT_BUFFER_SIZE    =   4096;

};



int     io_read_data( void *opaque, uint8_t *buffer, int size );




#endif