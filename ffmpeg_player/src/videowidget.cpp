#include "videowidget.h"

#include <QKeyEvent>



/*******************************************************************************
VideoWidget::VideoWidget()
********************************************************************************/
VideoWidget::VideoWidget( QWidget *parent )
    :   QVideoWidget(parent)
{}





/*******************************************************************************
VideoWidget::~VideoWidget()
********************************************************************************/
VideoWidget::~VideoWidget()
{
    QVideoWidget::~QVideoWidget();
}






/*******************************************************************************
VideoWidget::keyPressEvent()
********************************************************************************/
void    VideoWidget::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
        case Qt::Key_F :
            bool    flag    =   isFullScreen();
            setFullScreen( !flag );
            this->setGeometry( QRect(70,70,1401,851) );  // 先寫死 之後改成能動態調整
            break;
    }
}
