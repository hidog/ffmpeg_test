#ifndef DEMUX_IO_H
#define DEMUX_IO_H

#include "demux.h"


class InputOutput;




class DemuxIO : public Demux 
{
public:
    DemuxIO();
    ~DemuxIO();

    DemuxIO( const DemuxIO& ) = delete;
    DemuxIO( DemuxIO&& ) = delete;

    DemuxIO& operator = ( const DemuxIO& ) = delete;
    DemuxIO& operator = ( DemuxIO&& ) = delete;


    int     open_input() override;
    void    set_IO( InputOutput* _io );

private:

    InputOutput     *IO     =   nullptr; // note: owener is player.

};



#endif