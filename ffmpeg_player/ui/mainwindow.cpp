#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMutex>

#include <QVideoWidget>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

#include "player.h"
#include <mutex>


#include "worker.h"
#include "video_worker.h"







/*******************************************************************************
MainWindow::MainWindow()
********************************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    set_signal_slot();

    worker.main_cb =  std::bind( &MainWindow::main_v_play, this, std::placeholders::_1 );

    video_mtx       =   new QMutex( QMutex::NonRecursive );
    view_data       =   new VideoData;

    worker          =   new Worker(this);
    video_worker    =   new VideoWorker(this);
    audio_worker    =   new AudioWorker(this);
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
MainWindow::main_v_play()
********************************************************************************/
void MainWindow::main_v_play(QImage* img)
{
    QVideoWidget* video_widget = ui->widget;

    video_widget->videoSurface()->present(*img);
    //video_widget->show();
}




/*******************************************************************************
MainWindow::recv_video_frame_slot()
********************************************************************************/
void MainWindow::recv_video_frame_slot()
{
    QVideoWidget* video_widget = ui->widget;

    /*static bool flag = false;
    if( flag == false )
    {
        flag = true;
        QVideoSurfaceFormat format( QSize(1280,720), QVideoFrame::Format_RGB24 );
        video_widget->videoSurface()->start(format);
    }*/


    //LOG

    static int last_index = 0;


    mtx.lock();

    if( g_v_data.index - last_index != 1 )
        printf( " g_v_data.index %d last_index %d\n",  g_v_data.index , last_index );
    last_index = g_v_data.index;

    //printf("v index = %d\n", g_v_data.index );
    video_widget->videoSurface()->present( g_v_data.frame );





    mtx.unlock();

    //video_widget->show();
}




/*******************************************************************************
MainWindow::set_signal_slot()
********************************************************************************/
void MainWindow::set_signal_slot()
{
    connect( ui->pushButton, &QPushButton::clicked, this, &MainWindow::load_slot );
    connect( &worker, &Worker::recv_video_frame_signal, this, &MainWindow::recv_video_frame_slot );
}







/*******************************************************************************
MainWindow::load_slot()
********************************************************************************/
void MainWindow::load_slot()
{
    QVideoWidget* video_widget = ui->widget;

    static bool flag = false;
    if( flag == false )
    {
        flag = true;
        QVideoSurfaceFormat format( QSize(1280,720), QVideoFrame::Format_RGB24 );
        video_widget->videoSurface()->start(format);
    }



    worker.start();

#if 1
    //QVideoWidget* videoWidget = ui->widget;
    //videoWidget->videoSurface() 
#else
    QMediaPlayer *player = new QMediaPlayer;
    player->setMedia( QMediaContent(QUrl::fromLocalFile("D:\\code\\test.mp4")) );
    player->play();
#endif
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
