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
    SrtIO();
    ~SrtIO();

    void    init() override;
    void    open() override;
    void    close() override;
    int     read( uint8_t *buf, int buf_size ) override;

    int     recv_handle();
    void    server_init();
    void    client_init();

private:

    constexpr static int    buf_size    =   2000;

    SRTSOCKET       serv    =   -1;
    SRTSOCKET       handle  =   -1;
    std::thread     *thr;


    int write_index = 0;
    int read_index = 0;
    RecvData rd[2000];
};



#endif