#include "srt_io.h"
#include "tool.h"
#include <srt.h>


// 避免重複呼叫 srt_startup. 若改成multi-thread,則需要修改這邊的設計.
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
    // multi-thread 的時候小心, 需要加 mutex lock, 或是用其他 global static function 處理
    if( srt_has_init == false )
    {
        srt_has_init    =   true;
        srt_startup();
        srt_setloglevel(srt_logging::LogLevel::debug);
    }

    EncodeSetting&  setting     =   get_encode_setting();

    //
    addrinfo    hints;
    addrinfo*   res;

    int     ret =   0;

    memset( &hints, 0, sizeof(struct addrinfo) );
    hints.ai_flags      =   AI_PASSIVE;
    hints.ai_family     =   AF_INET;
    hints.ai_socktype   =   SOCK_DGRAM;

    ret     =   getaddrinfo( NULL, setting.srt_port.c_str(), &hints, &res );
    if( 0 != ret )    
        MYLOG( LOG::L_ERROR, "getaddrinfo fail." );    

    //
    serv    =   srt_create_socket();
    if( serv == SRT_INVALID_SOCK )
        MYLOG( LOG::L_ERROR, "create serv fail." );

    SRT_TRANSTYPE   live_mode   =   SRTT_LIVE;
    srt_setsockopt( serv, 0, SRTO_TRANSTYPE, &live_mode, sizeof(live_mode) );

    ret     =   srt_bind(serv, res->ai_addr, res->ai_addrlen);
    if( SRT_ERROR == ret )
        MYLOG( LOG::L_ERROR, "bind fail." );

    freeaddrinfo(res);
}





/*******************************************************************************
SrtIO::need_wait()
********************************************************************************/
bool    SrtIO::need_wait()
{
    int32_t     send_buf_size   =   0;
    int32_t     data_in_buf     =   0;
    int         len     =   0;

    srt_getsockopt( handle, 0, SRTO_SNDBUF,  &send_buf_size, &len );
    srt_getsockopt( handle, 0, SRTO_SNDDATA, &data_in_buf, &len );
    
    if( data_in_buf > 3*send_buf_size/4 )
        MYLOG( LOG::L_WARN, "data_in_buf = %d, 3*send_buf_size/4 = %d\n", data_in_buf, 3*send_buf_size/4 );

    return  data_in_buf > 3*send_buf_size/4;
}






/*******************************************************************************
SrtIO::is_connect()
********************************************************************************/
bool    SrtIO::is_connect()
{
    if( handle == SRT_INVALID_SOCK )
        return  false;
    else
        return  true;
}




/*******************************************************************************
SrtIO::is_connect()
********************************************************************************/
bool    SrtIO::is_stop()
{
    if( handle == SRT_INVALID_SOCK )
        return  true;
    else
        return  false;
}





/*******************************************************************************
SrtIO::client_init()
********************************************************************************/
void    SrtIO::client_init()
{
    // multi-thread 的時候小心, 需要加 mutex lock, 或是用其他 global static function 處理
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
    {
        server_init();
    }
}







/*******************************************************************************
SrtIO::client_open()
********************************************************************************/
void    SrtIO::client_open()
{    
    DecodeSetting&  setting     =   get_decode_setting();

    addrinfo    hints, *peer = nullptr;

    memset( &hints, 0, sizeof(struct addrinfo) );
    hints.ai_flags      =   AI_PASSIVE;
    hints.ai_family     =   AF_INET;
    hints.ai_socktype   =   SOCK_DGRAM;

    int     ret     =   0;

    ret     =   getaddrinfo( setting.srt_ip.c_str(), setting.srt_port.c_str(), &hints, &peer );
    if( 0 != ret )
        MYLOG( LOG::L_ERROR, "get addr fail." );

    assert( handle == SRT_INVALID_SOCK );
    handle  =   srt_create_socket();
    if( handle == SRT_INVALID_SOCK )
        MYLOG( LOG::L_ERROR, "create fail." );

    SRT_TRANSTYPE   live_mode   =   SRTT_LIVE;
    srt_setsockopt( handle, 0, SRTO_TRANSTYPE, &live_mode, sizeof live_mode );

    int64_t maxbw = -1; 
    srt_setsockopt( handle, 0, SRTO_MAXBW, &maxbw, sizeof maxbw );

    int recv_buf = 62768000;  // default = 8192
    srt_setsockopt( handle, 0, SRTO_RCVBUF, &recv_buf, sizeof recv_buf );

    //int send_buf = 62768000;  // default = 8192
    //srt_setsockopt( handle, 0, SRTO_SNDBUF, &send_buf, sizeof send_buf );

    // note: 未來加入連不上線的timeout處理
    while(true)
    {
        ret     =   srt_connect( handle, peer->ai_addr, peer->ai_addrlen );

        if( SRT_ERROR != ret ) 
            break;

        MYLOG( LOG::L_DEBUG, "connect fail. ret = %d\n", ret );
    }
    freeaddrinfo(peer);
    MYLOG( LOG::L_INFO, "connected." );

    // start
    assert( thr == nullptr );
    thr     =   new std::thread( &SrtIO::recv_handle, this );
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
        handle  =   srt_accept(serv, (sockaddr*)&clientaddr, &addrlen);
        if (SRT_INVALID_SOCK == handle)
        {
            MYLOG( LOG::L_INFO, "accept fail." );
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

    //thr =   new std::thread( &SrtIO::recv_handle, this );
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
    MYLOG( LOG::L_INFO, "start recv." );
    int     res =   0;

    while( is_end == false )
    {
        if( next_index(write_index) == read_index )
        {
            MYLOG( LOG::L_WARN, "buffer full!!" );
            SLEEP_10MS;
        }

        res     =   srt_recvmsg( handle, rd[write_index].data, 1316 );
        rd[write_index].size    =   res;
        write_index             =   next_index(write_index);

        //MYLOG( LOG::L_DEBUG, "recv size = %d", res );
        if( res <= 0 )
        {
            MYLOG( LOG::L_INFO, "recv no data." );
            is_end  =   true;  // 利用設置 flag 來跳出迴圈. ret <= 0 通常是斷線造成.
        }
    }

     //
    srt_close(handle);
    handle  =   SRT_INVALID_SOCK;
    MYLOG( LOG::L_INFO, "recv_handle finish." );
    return  1;
}





/*******************************************************************************
SrtIO::read()
********************************************************************************/
int     SrtIO::read( uint8_t *buffer, int size )
{
    while( read_index == write_index )
    {
        // no data.
        if( handle == SRT_INVALID_SOCK )
            return  EOF;
        SLEEP_1MS;
    }

    if( rd[read_index].size <= 0 )
        return  EOF;  // 代表已經斷線了沒資料.

    // 如果需要的資料太少, 跳錯誤. 目前並沒有處理這樣的 case.
    // 理論上 size >= 1316.
    if( rd[read_index].size > size )
        MYLOG( LOG::L_ERROR, "wanted size = %d too small.", size );

    //
    int     read_size   =   0;
    int     remain      =   size;
    while( true )
    {
        if( remain < rd[read_index].size || rd[read_index].size <= 0 )
            break;
        
        // 資料夠就多讀幾筆.
        memcpy( buffer + read_size, rd[read_index].data, rd[read_index].size );
        read_size   +=  rd[read_index].size;
        remain      -=  rd[read_index].size;
        read_index  =   next_index(read_index);

        if( read_index == write_index )
            break;
    }

    //MYLOG( LOG::L_DEBUG, "wanted size = %d, read size = %d", size, read_size );
    return  read_size;
}





/*******************************************************************************
SrtIO::close()
********************************************************************************/
void    SrtIO::close()
{
    IO_Direction    dir =   get_direction();

    if( dir == IO_Direction::RECV )
        client_end();
    else
        server_end();
}






/*******************************************************************************
SrtIO::client_end()
********************************************************************************/
void    SrtIO::client_end()
{
    is_end  =   true;
    thr->join();
    handle  =   SRT_INVALID_SOCK;
    MYLOG( LOG::L_INFO, "close socket.");

    is_end  =   false;

    write_index =   0;
    read_index  =   0;
    delete [] rd;
    rd  =   nullptr;
}






/*******************************************************************************
SrtIO::server_end()
********************************************************************************/
void    SrtIO::server_end()
{
    int32_t     data_in_buf    =   0;
    int         len;

    // flush
    while(true)
    {
        srt_getsockopt( handle, 0, SRTO_SNDDATA, &data_in_buf, &len );
        if( data_in_buf == 0 )
            break;        
        //MYLOG( LOG::L_DEBUG, "data_in_buf = %d", data_in_buf );
    }

    MYLOG( LOG::L_INFO, "server end." );

    srt_close(handle);  // note: call srt_close, it will not send data in buffer.
    handle  =   SRT_INVALID_SOCK;

    srt_close(serv);
    serv    =   SRT_INVALID_SOCK;

    srt_cleanup();
}





/*******************************************************************************
SrtIO::write()
********************************************************************************/
int     SrtIO::write( uint8_t *buffer, int size )
{
    //MYLOG( LOG::L_DEBUG, "buf size = %d", buf_size );

    int     ret         =   0;
    int     send_size   =   0;
    int     remain      =   size;

    while(true)
    {
        if( remain > 1316 )
        {
            ret     =   srt_send( handle, (const char*)(buffer + send_size), 1316 );
            // 暫時不作斷線檢查. 如果 remote disconnect 會造成 crash.
            if( ret != 1316 )
                MYLOG( LOG::L_ERROR, "ret = %d", ret );
            send_size   +=  1316;
            remain      -=  1316;
        }
        else
        {
            ret     =   srt_send( handle, (const char*)(buffer + send_size), remain );
            if( ret != remain )
                MYLOG( LOG::L_ERROR, "ret = %d", ret );
            break;
        }
    }
    
    return  ret;
}



