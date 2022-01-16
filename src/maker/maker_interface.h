#ifndef MAKER_INTERFACE_H
#define MAKER_INTERFACE_H


#include "tool.h"


class DLL_API MakerInterface
{
public:

    virtual void    work()          =   0;
    virtual void    end()           =   0;
    virtual bool    is_connect()    =   0;
};



DLL_API void    maker_encode_example();



#endif