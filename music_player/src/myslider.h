#ifndef MY_SLIDER_H
#define MY_SLIDER_H

#include <QSlider>


class MySlider : public QSlider 
{
public:

    MySlider( QWidget *parent = nullptr );
    MySlider( Qt::Orientation orientation, QWidget *parent = nullptr );
    ~MySlider();

    bool    is_mouse_press();

    void    mousePressEvent( QMouseEvent *ev ) override;
    void    mouseMoveEvent( QMouseEvent *ev ) override;
    void    mouseReleaseEvent( QMouseEvent *ev ) override;
    
    int     mouse_move( QMouseEvent *ev );

private:

    bool    mouse_pressed   =   false;

};



#endif