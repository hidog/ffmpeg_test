#include "input_output.h"
#include "file_io.h"
#include "srt_io.h"





/*******************************************************************************
InputOutput::InputOutput()
********************************************************************************/
InputOutput::InputOutput( IO_Direction dir )
    :   direction(dir)
{}




/*******************************************************************************
InputOutput::~InputOutput()
********************************************************************************/
InputOutput::~InputOutput()
{}





/*******************************************************************************
create_IO
********************************************************************************/
InputOutput*    create_IO( IO_Type io_type, IO_Direction dir )
{
    InputOutput     *ptr    =   nullptr;

    switch( io_type )
    {
    case IO_Type::FILE_IO :
        ptr     =   new FileIO{ dir };
        break;
    case IO_Type::SRT_IO:
        ptr     =   new SrtIO{ dir };
        break;
    default:
        assert(0);
    }

    return  ptr;
}





/*******************************************************************************
InputOutput::get_decode_setting
********************************************************************************/
DecodeSetting&   InputOutput::get_decode_setting()
{
    return  decode_setting;
}





/*******************************************************************************
InputOutput::get_encode_setting
********************************************************************************/
EncodeSetting&   InputOutput::get_encode_setting()
{
    return  encode_setting;
}





/*******************************************************************************
InputOutput::get_setting
********************************************************************************/
IO_Direction    InputOutput::get_direction()
{
    return  direction;
}





/*******************************************************************************
InputOutput::set_decode
********************************************************************************/
void    InputOutput::set_decode( DecodeSetting _setting )
{
    decode_setting     =   _setting;
}






/*******************************************************************************
InputOutput::set_encode
********************************************************************************/
void    InputOutput::set_encode( EncodeSetting _setting )
{
    encode_setting     =   _setting;
}
