#include "file_io.h"
#include "tool.h"



/*******************************************************************************
FileIO::FileIO()
********************************************************************************/
FileIO::FileIO( IO_Direction dir )
    :   InputOutput(dir)
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
FileIO::init()
********************************************************************************/
void    FileIO::close()
{
    fclose(fp);
    fp  =   nullptr;
}




/*******************************************************************************
FileIO::is_connect()
********************************************************************************/
bool    FileIO::is_connect()
{
    MYLOG( LOG::WARN, "io type is file." );
    return  true;
}





/*******************************************************************************
FileIO::is_stop()
********************************************************************************/
bool    FileIO::is_stop()
{
    return  feof(fp);
}



    
/*******************************************************************************
FileIO::open()
********************************************************************************/
void    FileIO::open()
{
    IO_Direction    dir         =   get_direction();

    if( dir == IO_Direction::RECV )
    {
        DecodeSetting&  setting     =   get_decode_setting();
        std::string     filename    =   setting.filename;

        MYLOG( LOG::INFO, "load file %s", filename.c_str() );
        fp  =   fopen( filename.c_str(), "rb" );
        if( fp == nullptr )
            MYLOG( LOG::ERROR, "open file fail." );
    }
    else
    {
        EncodeSetting&  setting     =   get_encode_setting();
        
        switch( IO_Type::FILE_IO )
        {
        case IO_Type::FILE_IO:
            assert( setting.filename.empty() == false );
            fp  =   fopen( setting.filename.c_str(), "wb+" );
            if( fp == nullptr )
                MYLOG( LOG::ERROR, "open file fail." );
            break;
        default:
            assert(0);
        }
    }
}





/*******************************************************************************
FileIO::read()
********************************************************************************/
int     FileIO::read( uint8_t *buffer, int size )
{
    if( fp == nullptr )
        MYLOG( LOG::ERROR, "fp not open." );

    if( feof(fp) != 0 )
        return  EOF;

    int ret     =   fread( buffer, 1, size, fp );

    return ret;
}





/*******************************************************************************
FileIO::write()
********************************************************************************/
int     FileIO::write( uint8_t *buf, int buf_size )
{
    if( fp == nullptr )
        MYLOG( LOG::ERROR, "fp not open." );

    int ret     =   fwrite( buf, 1, buf_size, fp );
    return ret;
}




