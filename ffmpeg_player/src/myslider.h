#ifndef MY_SLIDER_H
#define MY_SLIDER_H

#include <QSlider>


class MySlider : public QSlider 
{
public:

    MySlider( QWidget *parent = nullptr );
    MySlider( Qt::Orientation orientation, QWidget *parent = nullptr );
    ~MySlider();

    void    mousePressEvent( QMouseEvent *ev ) override;
    void    mouseMoveEvent( QMouseEvent *ev ) override;
    void    mouseReleaseEvent( QMouseEvent *ev ) override;
    
    void    mouse_move( QMouseEvent *ev );

private:

};



#endif