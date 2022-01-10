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
    void    set_IO( InputOutput* _io );

    int     init() override;    
    int     end() override;

private:

    InputOutput     *IO     =   nullptr;
    DecodeSetting   setting;

};



#endif