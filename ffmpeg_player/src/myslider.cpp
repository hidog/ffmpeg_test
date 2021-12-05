#include "myslider.h"

#include <QMouseEvent>
#include "tool.h"



/*******************************************************************************
MySlider::MySlider()
********************************************************************************/
MySlider::MySlider( QWidget *parent )
    :   QSlider(parent)
{}




/*******************************************************************************
MySlider::MySlider()
********************************************************************************/
MySlider::MySlider( Qt::Orientation orientation, QWidget *parent )
    :   QSlider(orientation,parent)
{}





/*******************************************************************************
MySlider::~MySlider()
********************************************************************************/
MySlider::~MySlider()
{}





/*******************************************************************************
MySlider::mousePressEvent()
********************************************************************************/
void    MySlider::mousePressEvent( QMouseEvent *ev )
{
    auto pos = ev->pos();
    MYLOG( LOG::DEBUG, "pos = %d %d ", pos.rx(), pos.ry() );

    if( orientation() == Qt::Horizontal )
        setSliderPosition( pos.rx() );
    else if( orientation() == Qt::Vertical )
        setSliderPosition( pos.rx() );
    else
        MYLOG( LOG::ERROR, "un defined");
}
