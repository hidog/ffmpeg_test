#include "input_output.h"
#include "file_io.h"
#include "srt_io.h"





/*******************************************************************************
InputOutput::InputOutput()
********************************************************************************/
InputOutput::InputOutput()
{}





/*******************************************************************************
InputOutput::~InputOutput()
********************************************************************************/
InputOutput::~InputOutput()
{}










/*******************************************************************************
create_IO
********************************************************************************/
InputOutput*    create_IO( IO_Type io_type )
{
    InputOutput     *ptr    =   nullptr;

    switch( io_type )
    {
    case IO_Type::FILE_IO :
        ptr     =   new FileIO;
        break;
    case IO_Type::SRT_IO:
        ptr     =   new SrtIO;
        break;
    default:
        assert(0);
    }

    return  ptr;
}





/*******************************************************************************
InputOutput::get_setting
********************************************************************************/
DecodeSetting   InputOutput::get_setting()
{
    return  setting;
}





/*******************************************************************************
InputOutput::set
********************************************************************************/
void    InputOutput::set( DecodeSetting _setting )
{
    setting     =   _setting;
}
