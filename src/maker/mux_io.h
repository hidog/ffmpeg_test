#ifndef MUX_IO_H
#define MUX_IO_H


#include "mux.h"



class MuxIO : public Mux
{
public:

    MuxIO();
    ~MuxIO();

    MuxIO( const MuxIO& ) = delete;
    MuxIO( MuxIO&& ) = delete;

    MuxIO& operator = ( const MuxIO& ) = delete;
    MuxIO& operator = ( MuxIO&& ) = delete;

private:


};



#endif