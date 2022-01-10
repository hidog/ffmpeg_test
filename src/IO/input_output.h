#ifndef INPUT_OUTPUT_H
#define INPUT_OUTPUT_H


#include "../player/play_def.h"


class InputOutput
{
public:
    InputOutput();
    virtual ~InputOutput();

    InputOutput( const InputOutput& ) = delete;
    InputOutput( InputOutput&& ) = delete;

    InputOutput& operator = ( const InputOutput& ) = delete;
    InputOutput& operator = ( InputOutput&& ) = delete;

    void    set( DecodeSetting _setting );

    virtual void    init()  =   0;
    virtual void    open()  =   0;
    virtual int     read( uint8_t *buf, int buf_size ) =  0;

protected:
    DecodeSetting   get_setting();

private:

    DecodeSetting   setting;


};




InputOutput*    create_IO( IO_Type io_type );



#endif