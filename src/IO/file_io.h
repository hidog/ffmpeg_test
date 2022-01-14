#ifndef FILE_IO_H
#define FILE_IO_H

#include "input_output.h"


class FileIO : public InputOutput
{
public:
    FileIO( IO_Direction dir );
    ~FileIO();

    FileIO( const FileIO& ) = delete;
    FileIO( FileIO&& ) = delete;

    FileIO& operator = ( const FileIO& ) = delete;
    FileIO& operator = ( FileIO&& ) = delete;

    void    init() override;
    void    open() override;
    int     read( uint8_t *buffer, int size ) override;
    int     write( uint8_t *buffer, int size ) override;
    void    close() override;
    bool    is_connect() override;

private:

    FILE    *fp     =   nullptr;

};



#endif