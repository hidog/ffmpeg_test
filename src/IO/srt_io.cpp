#include "srt_io.h"
#include "tool.h"
#include <srt.h>


#undef ERROR  // ���F�קK��Y�� header ��define�_�Ĭ� 

// �קK���ƩI�s srt_startup. �Y�令multi-thread,�h�ݭn�ק�o�䪺�]�p.
static bool     srt_has_init    =   false;





/*******************************************************************************
SrtIO::SrtIO()
********************************************************************************/
SrtIO::SrtIO( IO_Direction dir )
    :   InputOutput(dir)
{}
  





/*******************************************************************************
SrtIO::~SrtIO()
********************************************************************************/
SrtIO::~SrtIO()
{}





/*******************************************************************************
SrtIO::server_init()
********************************************************************************/
void    SrtIO::server_init()
{
    // multi-thread ���ɭԤp��, �ݭn�[ mutex lock, �άO�Ψ�L global static function �B�z
    if( srt_has_init == false )
    {
        srt_has_init    =   true;
        srt_startup();
        srt_setloglevel(srt_logging::LogLevel::debug);
    }

    //
    addrinfo    hints;
    addrinfo*   res;

    int     ret =   0;

    memset( &hints, 0, sizeof(struct addrinfo) );
    hints.ai_flags      =   AI_PASSIVE;
    hints.ai_family     =   AF_INET;
    hints.ai_socktype   =   SOCK_DGRAM;

    ret     =   getaddrinfo( NULL, "1234", &hints, &res );
    if( 0 != ret )    
        MYLOG( LOG::ERROR, "getaddrinfo fail." );    

    //
    serv    =   srt_create_socket();
    if( serv == SRT_INVALID_SOCK )
        MYLOG( LOG::ERROR, "create serv fail." );

    SRT_TRANSTYPE   live_mode   =   SRTT_LIVE;
    srt_setsockopt( serv, 0, SRTO_TRANSTYPE, &live_mode, sizeof(live_mode) );

    ret     =   srt_bind(serv, res->ai_addr, res->ai_addrlen);
    if( SRT_ERROR == ret )
        MYLOG( LOG::ERROR, "bind fail." );

    freeaddrinfo(res);
}





/*******************************************************************************
SrtIO::client_init()
********************************************************************************/
void    SrtIO::client_init()
{
    // multi-thread ���ɭԤp��, �ݭn�[ mutex lock, �άO�Ψ�L global static function �B�z
    if( srt_has_init == false )
    {
        srt_has_init    =   true;
        srt_startup();
        srt_setloglevel(srt_logging::LogLevel::debug);
    }
}






/*******************************************************************************
SrtIO::init()
********************************************************************************/
void    SrtIO::init()
{
    IO_Direction    dir     =   get_direction();

    is_end  =   false;

    if( dir == IO_Direction::RECV )
    {
        client_init();        
        write_index =   0;
        read_index  =   0;
        rd  =   new RecvData[buf_size];
    }
    else
        server_init();
}







/*******************************************************************************
SrtIO::client_open()
********************************************************************************/
void    SrtIO::client_open()
{    
    DecodeSetting&  setting     =   get_setting();

    addrinfo    hints, *peer = nullptr;

    memset( &hints, 0, sizeof(struct addrinfo) );
    hints.ai_flags      =   AI_PASSIVE;
    hints.ai_family     =   AF_INET;
    hints.ai_socktype   =   SOCK_DGRAM;

    int     ret     =   0;

    ret     =   getaddrinfo( setting.srt_ip.c_str(), setting.srt_port.c_str(), &hints, &peer );
    if( 0 != ret )
        MYLOG( LOG::ERROR, "get addr fail." );

    assert( handle == SRT_INVALID_SOCK );
    handle  =   srt_create_socket();
    if( handle == SRT_INVALID_SOCK )
        MYLOG( LOG::ERROR, "create fail." );

    SRT_TRANSTYPE   live_mode   =   SRTT_LIVE;
    srt_setsockopt( handle, 0, SRTO_TRANSTYPE, &live_mode, sizeof live_mode );

    int64_t maxbw = -1; 
    srt_setsockopt( handle, 0, SRTO_MAXBW, &maxbw, sizeof maxbw );

    int recv_buf = 62768000;  // default = 8192
    srt_setsockopt( handle, 0, SRTO_RCVBUF, &recv_buf, sizeof recv_buf );

    //int send_buf = 62768000;  // default = 8192
    //srt_setsockopt( handle, 0, SRTO_SNDBUF, &send_buf, sizeof send_buf );

    // note: ���ӥ[�J�s���W�u��timeout�B�z
    while(true)
    {
        ret     =   srt_connect( handle, peer->ai_addr, peer->ai_addrlen );

        if( SRT_ERROR != ret ) 
            break;

        MYLOG( LOG::DEBUG, "connect fail. ret = %d\n", ret );
    }
    freeaddrinfo(peer);
    MYLOG( LOG::INFO, "connected." );

    // start
    assert( thr == nullptr );
    thr     =   new std::thread( &SrtIO::recv_handle, this );

        //srt_close(fhandle);
        //break;    

}





    
/*******************************************************************************
SrtIO::server_open()
********************************************************************************/
void    SrtIO::server_open()
{
    // start connect.
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







/*******************************************************************************
SrtIO::open()
********************************************************************************/
void    SrtIO::open()
{
    IO_Direction    dir     =   get_direction();

    if( dir == IO_Direction::RECV )
        client_open();
    else
        server_open();
}




/*******************************************************************************
SrtIO::next_write_index()
********************************************************************************/
int     SrtIO::next_index( int w )
{
    return  (w + 1) % buf_size;
}





/*******************************************************************************
SrtIO::recv_handle()
********************************************************************************/
int     SrtIO::recv_handle()
{
    MYLOG( LOG::INFO, "start recv." );
    int     res =   0;

    while( is_end == false )
    {
        if( next_index(write_index) == read_index )
        {
            MYLOG( LOG::WARN, "buffer full!!" );
            SLEEP_10MS;
        }

        res     =   srt_recvmsg( handle, rd[write_index].data, 1316 );
        rd[write_index].size    =   res;
        write_index             =   next_index(write_index);

        //MYLOG( LOG::DEBUG, "recv size = %d", res );

        if( res <= 0 )
        {
            MYLOG( LOG::INFO, "recv no data." );
            break;
        }
    }

    // �]���i��O�_�u�h�X�j��.
    is_end  =   true;
    srt_close(handle);
    handle  =   SRT_INVALID_SOCK;


    MYLOG( LOG::INFO, "recv_handle finish." );


    return  1;
}





/*******************************************************************************
SrtIO::close()
********************************************************************************/
void    SrtIO::close()
{
    IO_Direction    dir =   get_direction();

    if( dir == IO_Direction::RECV )
        client_end();
    //else
      //  server_end();
}






/*******************************************************************************
SrtIO::client_end()
********************************************************************************/
void    SrtIO::client_end()
{
    is_end  =   true;
    thr->join();
    handle  =   SRT_INVALID_SOCK;
    MYLOG( LOG::INFO, "close socket.");

    is_end  =   false;

    write_index =   0;
    read_index  =   0;
    delete [] rd;
    rd  =   nullptr;
}





/*******************************************************************************
SrtIO::read()
********************************************************************************/
int     SrtIO::read( uint8_t *buf, int buf_size )
{
    int read_size   =   0;

    if( handle == -1 || is_end == true )
        return -1;

    while( read_index == write_index )
    {
        // no data.
        if( handle == -1 || is_end == true )
            return -1;
        SLEEP_10MS;
    }

    if( rd[read_index].size <= 0 )
        return  -1;  // �N��w�g�_�u�F�S���.
    memcpy( buf, rd[read_index].data, rd[read_index].size );

    read_size   =   rd[read_index].size;
    read_index  =   next_index(read_index);

    return  read_size;
}





#define ERROR 0   // �Ѧ� #undef ������