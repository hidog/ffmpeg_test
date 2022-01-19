#ifndef VIDEO_WIDGET_H
#define VIDEO_WIDGET_H


#include <QVideoWidget>


class VideoWidget : public QVideoWidget
{
public:
    VideoWidget( QWidget *parent = nullptr );
    ~VideoWidget();

    void    keyPressEvent( QKeyEvent *event );

private:
	
};


#endif