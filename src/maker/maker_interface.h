#ifndef MAKER_INTERFACE_H
#define MAKER_INTERFACE_H


#include "tool.h"


/*
    為了避免 UI 端 include 到 ffmpeg, 將 maker 的 interface 切割出來.
    player 端也可以考慮類似的設計, 但目前尚無需求.
*/


class DLL_API MakerInterface
{
public:
    MakerInterface() {}
    virtual ~MakerInterface() {}

    virtual void    work()          =   0;
    virtual void    end()           =   0;
    virtual bool    is_connect()    =   0;

};



DLL_API void    maker_encode_example();
DLL_API void    output_by_io( MediaInfo media_info, std::string _port, MakerInterface* maker );

DLL_API MakerInterface* create_maker_io();


#endif