#include "srt_io.h"
#include "tool.h"
#include <srt.h>


#undef ERROR


SrtIO::SrtIO()
    :   InputOutput()
{
    static bool has_init = false;

    // multi-thread 的時候小心, 需要加 mutex lock, 或是用其他global static function 處理
    if( has_init == false )
    {
        has_init    =   true;
        srt_startup();
        srt_setloglevel(srt_logging::LogLevel::debug);
    }

    addrinfo    hints;
    addrinfo*   res;

    memset( &hints, 0, sizeof(struct addrinfo) );
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if( 0 != getaddrinfo(NULL, "1234", &hints, &res) )    
        MYLOG( LOG::ERROR, "getaddrinfo fail." );
    

    serv = srt_create_socket();

    SRT_TRANSTYPE live_mode = SRTT_LIVE;
    srt_setsockopt( serv, 0, SRTO_TRANSTYPE, &live_mode, sizeof(live_mode) );

    if( SRT_ERROR == srt_bind(serv, res->ai_addr, res->ai_addrlen) )
        MYLOG( LOG::ERROR, "bind fail." );

    freeaddrinfo(res);

    write_index = 0;
    read_index = 0;




    srt_listen(serv, 1000);

    sockaddr_storage clientaddr;
    int addrlen = sizeof(clientaddr);


    while (true)
    {
        handle = srt_accept(serv, (sockaddr*)&clientaddr, &addrlen);
        if (SRT_INVALID_SOCK == handle)
        {
            MYLOG( LOG::INFO, "accept fail." );
        }
        else
            break;
    }

    int send_buf = 62768000;  // default = 8192
    srt_setsockopt( handle, 0, SRTO_SNDBUF, &send_buf, sizeof send_buf );

    int recv_buf = 62768000;  // default = 8192
    srt_setsockopt( handle, 0, SRTO_RCVBUF, &recv_buf, sizeof recv_buf );

    int64_t maxbw = 0; 
    srt_setsockopt( handle, 0, SRTO_MAXBW, &maxbw, sizeof maxbw );

    thr =   new std::thread( &SrtIO::recv_handle, this );
}
  



SrtIO::~SrtIO()
{}




void    SrtIO::init()
{


}



void    SrtIO::open()
{

}






int     SrtIO::recv_handle()
{
    MYLOG( LOG::INFO, "start recv." );
    int res = 0;

    while(true)
    {
        if( (write_index+1)%500 == read_index )
        {
            MYLOG( LOG::WARN, "buffer full!!" );
            SLEEP_10MS;
        }

        res = srt_recvmsg( handle, rd[write_index].data, 1316 );
        rd[write_index].size = res;
        write_index = (write_index+1)%500;

        //MYLOG( LOG::DEBUG, "recv size = %d", res );

        if( res <= 0 )
            break;
    }

    MYLOG( LOG::INFO, "end recv.");
    return  1;
}




void    SrtIO::close()
{
    thr->join();
}






int     SrtIO::read( uint8_t *buf, int buf_size )
{
    while( read_index == write_index )
        SLEEP_10MS;


    memcpy( buf, rd[read_index].data, rd[read_index].size );
    int read_size = rd[read_index].size;
    read_index = (read_index+1)%500;
    return  read_size;
}




#define ERROR 0