#ifndef DEMUX_IO_H
#define DEMUX_IO_H

#include "demux.h"



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


private:
};



#endif