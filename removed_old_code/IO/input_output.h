#ifndef INPUT_OUTPUT_H
#define INPUT_OUTPUT_H


#include "../player/play_def.h"
#include "../maker/maker_def.h"


/*
https://www.twblogs.net/a/5ef50c480cb8aa77788390c9   RTP samplpe code  如果以後要做RTP再參考
*/


class InputOutput
{
public:
    InputOutput( IO_Direction dir );
    virtual ~InputOutput();

    InputOutput( const InputOutput& ) = delete;
    InputOutput( InputOutput&& ) = delete;

    InputOutput& operator = ( const InputOutput& ) = delete;
    InputOutput& operator = ( InputOutput&& ) = delete;

    void    set_decode( DecodeSetting _setting );
    void    set_encode( EncodeSetting _setting );

    virtual void    init()  =   0;
    virtual void    open()  =   0;
    virtual void    close() =   0;
    virtual bool    is_stop()       =   0;
    virtual bool    is_connect()    =   0;
    virtual int     read( uint8_t *buf, int buf_size )  =  0;
    virtual int     write( uint8_t *buf, int buf_size ) =  0;

protected:
    DecodeSetting&  get_decode_setting();
    EncodeSetting&  get_encode_setting();
    IO_Direction    get_direction();

private:
    DecodeSetting   decode_setting;
    EncodeSetting   encode_setting;
    IO_Direction    direction;

};




InputOutput*    create_IO( IO_Type io_type, IO_Direction dir );



#endif