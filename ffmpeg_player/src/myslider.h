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

private:

};



#endif