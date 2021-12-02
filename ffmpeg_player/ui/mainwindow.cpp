#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMutex>
#include <QMessageBox>

#include <QVideoWidget>

#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

#include <QFileDialog>
#include <QDebug>
#include <QKeyEvent>

#include <mutex>

#include "player.h"

#include "worker.h"
#include "video_worker.h"
#include "audio_worker.h"






/*******************************************************************************
MainWindow::MainWindow()
********************************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 必須註冊自定義的物件.
    qRegisterMetaType<VideoSetting>("VideoSetting");

    //video_widget    =   new QVideoWidget();

    //
    video_mtx       =   new QMutex( QMutex::NonRecursive );
    view_data       =   new VideoData;
    view_data->index        =   0;
    view_data->timestamp    =   0;

    worker          =   new Worker(this);
    video_worker    =   new VideoWorker(this);
    audio_worker    =   new AudioWorker(this);

    set_signal_slot();

}




/*******************************************************************************
MainWindow::~MainWindow()
********************************************************************************/
MainWindow::~MainWindow()
{
    //worker.terminate();
    delete ui;

    /*if( video_widget->isActiveWindow() )
        video_widget->close();
    delete video_widget;*/

    delete worker;
    delete video_worker;
    delete audio_worker;

    delete video_mtx;
    delete view_data;
}






/*******************************************************************************
MainWindow::recv_video_frame_slot()
********************************************************************************/
void MainWindow::recv_video_frame_slot()
{ 
    /*if( video_widget->isVisible() == false )
        return;*/

    QVideoWidget    *video_widget   =   ui->videoWidget;

    static int  last_index  =   0;

    //
    video_mtx->lock();

    if( view_data->index - last_index != 1 )
        MYLOG( LOG::WARN, "vidw_data.index = %d, last_index = %d",  view_data->index , last_index );

    // 未來考慮改成timer驅動,方便確認是否有frame來不及解出來.
    // 未必真的用timer, 可以用loop控制
    // 計算每個frame出現的時間, 超過就秀frame
    // seek的時候, 把出現時間歸零後重新計算. frame count 跟真正在影片中的frame count脫鉤,而是每次seek播放後重新計算frame count.

    last_index = view_data->index;
    video_widget->videoSurface()->present( view_data->frame );

    video_mtx->unlock();
}




/*******************************************************************************
MainWindow::set_signal_slot()
********************************************************************************/
void MainWindow::set_signal_slot()
{
    // 舊式寫法
    connect(    worker,     SIGNAL(video_setting_signal(VideoSetting)),     this,   SLOT(set_video_setting_slot(VideoSetting))  );
    //connect(    worker,           &Worker::video_setting_singal,          this,   &MainWindow::set_video_setting_slot );
    connect(    worker,             &Worker::subtitle_list_signal,          this,   &MainWindow::set_subtitle_list_slot         );

    connect(    ui->startButton,    &QPushButton::clicked,                  this,   &MainWindow::start_slot                     );
    connect(    ui->loadFileButton, &QPushButton::clicked,                  this,   &MainWindow::load_file_slot                 );
    connect(    video_worker,       &VideoWorker::recv_video_frame_signal,  this,   &MainWindow::recv_video_frame_slot          );
    connect(    ui->subCBox,        &QComboBox::currentTextChanged,         this,   &MainWindow::switch_subtitle_slot           );
}




/*******************************************************************************
MainWindow::set_video_setting_slot()
********************************************************************************/
void    MainWindow::set_video_setting_slot( VideoSetting vs )
{
    /*if( video_widget->videoSurface()->isActive() )
        video_widget->videoSurface()->stop();*/

    QVideoWidget    *video_widget   =   ui->videoWidget;


    QSize   size { vs.width, vs.height };

    QVideoSurfaceFormat     format { size, QVideoFrame::Format_RGB24 };
    video_widget->videoSurface()->start(format);   
    //video_widget->show();

    worker->finish_set_video();
}





/*******************************************************************************
MainWindow::start_slot()
********************************************************************************/
void MainWindow::start_slot()
{
    if( worker->is_set_src_file() == false )
    {
        QMessageBox::warning( this, tr("ffmpeg player"),
                                    tr("need choose file") );
        return;
    }

    //
    worker->start();
}



/*******************************************************************************
MainWindow::load_slot()
********************************************************************************/
void MainWindow::load_file_slot()
{
    QString filename     =   QFileDialog::getOpenFileName( this, tr("select src file"), "D:\\" );
    MYLOG( LOG::INFO, "load file %s", filename.toStdString().c_str() );
    worker->set_src_file(filename.toStdString());
}





/*******************************************************************************
MainWindow::get_video_mutex()
********************************************************************************/
QMutex*     MainWindow::get_video_mutex()
{
    return  video_mtx;
}




/*******************************************************************************
MainWindow::get_view_data()
********************************************************************************/
VideoData*   MainWindow::get_view_data()
{
    return  view_data;
}





/*******************************************************************************
MainWindow::get_worker()
********************************************************************************/
Worker*     MainWindow::get_worker()
{
    return  worker;
}




/*******************************************************************************
MainWindow::get_view_data()
********************************************************************************/
VideoWorker*    MainWindow::get_video_worker()
{
    return  video_worker;
}





/*******************************************************************************
MainWindow::get_audio_worker()
********************************************************************************/
AudioWorker*    MainWindow::get_audio_worker()
{
    return  audio_worker;
}




/*******************************************************************************
MainWindow::keyPressEvent()
********************************************************************************/
void    MainWindow::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
        case Qt::Key_F :
            QVideoWidget    *video_widget   =   ui->videoWidget;
            bool    flag    =   video_widget->isFullScreen();
            video_widget->setFullScreen( !flag );
            break;
    }
}




/*******************************************************************************
MainWindow::set_subtitle_list_slot()
********************************************************************************/
void    MainWindow::set_subtitle_list_slot( QStringList list )
{
    int     i;
    auto    sub_combobox    =   ui->subCBox;

    for( i = 0; i < list.size(); i++ )
        sub_combobox->addItem( list.at(i) );
}





/*******************************************************************************
MainWindow::switch_subtitle_slot()
********************************************************************************/
void    MainWindow::switch_subtitle_slot( QString path )
{
    qDebug() << path;
    worker->switch_subtitle(path);
}
