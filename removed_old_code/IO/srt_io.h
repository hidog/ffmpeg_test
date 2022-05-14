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


/*
https://srtlab.github.io/srt-cookbook/apps/ffmpeg/  build ffmpeg with srt
參考範例
ffmpeg -f lavfi -re -i smptebars=duration=60:size=1280x720:rate=30 -f lavfi -re \
-i sine=frequency=1000:duration=60:sample_rate=44100 -pix_fmt yuv420p \
-c:v libx264 -b:v 1000k -g 30 -keyint_min 120 -profile:v baseline \
-preset veryfast -f mpegts "srt://127.0.0.1:4200?pkt_size=1316"

這邊是自己加進 SRT, 沒有直接使用編譯選項
*/


class SrtIO : public InputOutput
{
public:
    SrtIO( IO_Direction dir );
    ~SrtIO();

    void    init() override;
    void    open() override;
    void    close() override;
    bool    is_connect() override;
    bool    is_stop() override;

    bool    need_wait();

    // use for client
    int     read( uint8_t *buffer, int size ) override;

    void    client_init();
    void    client_open();
    int     recv_handle();
    int     next_index( int w );
    void    client_end();

    // use for server.
    int     write( uint8_t *buf, int buf_size ) override;

    void    server_init();
    void    server_open();
    void    server_end();


private:
    bool    is_end  =   false;

    SRTSOCKET       handle  =   -1;
    std::thread     *thr    =   nullptr;

    // use for client
    constexpr static int    buf_size    =   10000;
    int     write_index =   0;
    int     read_index  =   0;
    RecvData    *rd =   nullptr;

    // use for server
    SRTSOCKET       serv    =   -1;

};



#endif