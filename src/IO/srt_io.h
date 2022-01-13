#ifndef SRT_IO_H
#define SRT_IO_H

#include "input_output.h"
#include <thread>


typedef int SRTSOCKET;



struct RecvData
{
    char    data[1316];
    int     size;
};



class SrtIO : public InputOutput
{
public:
    SrtIO( IO_Direction dir );
    ~SrtIO();

    void    init() override;
    void    open() override;
    void    close() override;

    // use for client
    int     read( uint8_t *buf, int buf_size ) override;

    void    client_init();
    void    client_open();
    int     recv_handle();
    int     next_index( int w );
    void    client_end();

    // use for server.
    int     write( uint8_t *buf, int buf_size ) override;

    void    server_init();
    void    server_open();

private:
    bool    is_end  =   false;

    SRTSOCKET       handle  =   -1;
    std::thread     *thr    =   nullptr;

    // use for client
    constexpr static int    buf_size    =   3500;
    int     write_index =   0;
    int     read_index  =   0;
    RecvData    *rd =   nullptr;

    // use for server
    SRTSOCKET       serv    =   -1;


};



#endif