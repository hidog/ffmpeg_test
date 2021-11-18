#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMutex>
#include <QMessageBox>

#include <QVideoWidget>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

#include <QFileDialog>

#include "player.h"
#include <mutex>


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

    //
    video_mtx       =   new QMutex( QMutex::NonRecursive );
    view_data       =   new VideoData;

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
    QVideoWidget*   video_widget    =   ui->widget;

    static int  last_index  =   -1;

    //
    video_mtx->lock();

    if( view_data->index - last_index != 1 )
        MYLOG( LOG::WARN, "vidw_data.index = %d, last_index = %d\n",  view_data->index , last_index );

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
    connect(    worker,     SIGNAL(video_setting_singal(VideoSetting)),     this,   SLOT(set_video_setting_slot(VideoSetting)) );
    //connect(    worker,     &Worker::video_setting_singal,     this,   &MainWindow::set_video_setting_slot );

    connect(    ui->startButton,    &QPushButton::clicked,                  this,   &MainWindow::start_slot );
    connect(    ui->loadFileButton, &QPushButton::clicked,                  this,   &MainWindow::load_file_slot );
    connect(    video_worker,       &VideoWorker::recv_video_frame_signal,  this,   &MainWindow::recv_video_frame_slot );
}




/*******************************************************************************
MainWindow::set_video_setting_slot()
********************************************************************************/
void    MainWindow::set_video_setting_slot( VideoSetting vs )
{
    QVideoWidget*   video_widget    =   ui->widget;

    if( video_widget->videoSurface()->isActive() )
        video_widget->videoSurface()->stop();

    QSize   size { vs.width, vs.height };

    QVideoSurfaceFormat     format { size, QVideoFrame::Format_RGB24 };
    video_widget->videoSurface()->start(format);

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
    QString file     =   QFileDialog::getOpenFileName( this, tr("select src file"), "D:\\" );
    worker->set_src_file(file.toStdString());
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
