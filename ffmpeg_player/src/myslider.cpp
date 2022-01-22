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
    mouse_pressed   =   true;
    mouse_move(ev);
}



/*******************************************************************************
MySlider::mouseMoveEvent()
********************************************************************************/
void    MySlider::mouseMoveEvent( QMouseEvent *ev )
{
    mouse_move(ev);
}



/*******************************************************************************
MySlider::mouseMoveEvent()
********************************************************************************/
bool    MySlider::is_mouse_press()
{
    return  mouse_pressed;
}






/*******************************************************************************
MySlider::mouseReleaseEvent()
********************************************************************************/
void    MySlider::mouseReleaseEvent( QMouseEvent *ev )
{
    mouse_pressed   =   false;

    int value   =   mouse_move(ev);
    if( hasTracking() == false )
        setValue(value);
}




/*******************************************************************************
MySlider::mouse_move()
********************************************************************************/
int    MySlider::mouse_move( QMouseEvent *ev )
{
    QPoint    pos   =   ev->pos();
    //MYLOG( LOG::L_DEBUG, "pos = %d %d %d %d", pos.rx(), pos.ry(), pos.x(), pos.y() );

    int     max     =   this->maximum();
    int     min     =   this->minimum();
    //int   sp      =   this->sliderPosition();
    int     width   =   this->width();
    int     height  =   this->height();

    double  rate;
    int     value;

    if( orientation() == Qt::Horizontal )
        rate = 1.0 * pos.rx() / width;
    else if( orientation() == Qt::Vertical )
        rate = 1.0 * pos.ry() / height;
    else
        MYLOG( LOG::L_ERROR, "un handle");

    value   =   rate * (max - min);
    value   =   value > max ? max : value;
    value   =   value < min ? min : value;

    setSliderPosition( value );

    return  value;
}
