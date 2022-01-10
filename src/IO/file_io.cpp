#include "file_io.h"
#include "tool.h"



/*******************************************************************************
FileIO::FileIO()
********************************************************************************/
FileIO::FileIO()
    :   InputOutput()
{}





/*******************************************************************************
FileIO::~FileIO()
********************************************************************************/
FileIO::~FileIO()
{}




/*******************************************************************************
FileIO::init()
********************************************************************************/
void    FileIO::init()
{}





    
/*******************************************************************************
FileIO::open()
********************************************************************************/
void    FileIO::open()
{
    std::string     filename    =   get_setting().filename;

    MYLOG( LOG::INFO, "load file %s", filename.c_str() );
    fp  =   fopen( filename.c_str(), "rb" );
    if( fp == nullptr )
        MYLOG( LOG::ERROR, "open file fail." );
}





/*******************************************************************************
FileIO::read()
********************************************************************************/
int     FileIO::read( uint8_t *buf, int buf_size )
{
    if( fp == nullptr )
        MYLOG( LOG::ERROR, "fp not open." );

    if( feof(fp) != 0 )
        return  EOF;

    int ret     =   fread( buf, 1, 4096, fp );

    if( buf_size != 4096 )
        printf( "read %d\n", buf_size );

    if( ret == 0 )
    {
        if( feof(fp) != 0 )
            return  EOF;
    }

    return ret;
}
