#ifndef MAKER_IO_H
#define MAKER_IO_H


#include "maker.h"


class InputOutput;



class MakerIO : public Maker
{
public:

    MakerIO();
    ~MakerIO();

    MakerIO( const MakerIO& ) = delete;
    MakerIO( MakerIO&& ) = delete;

    MakerIO& operator = ( const MakerIO& ) = delete;
    MakerIO& operator = ( MakerIO&& ) = delete;

    void    init( EncodeSetting _setting, VideoEncodeSetting v_setting, AudioEncodeSetting a_setting );
    void    init_IO();
    
    void    work() override;
    void    end() override;
    bool    is_connect() override;

private:

    InputOutput     *IO     =   nullptr;

};


#endif