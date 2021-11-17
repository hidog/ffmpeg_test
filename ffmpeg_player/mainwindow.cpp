#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QVideoWidget>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

#include "player.h"
#include <mutex>



VideoData g_v_data;
std::mutex mtx;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    set_signal_slot();

    worker.main_cb =  std::bind( &MainWindow::main_v_play, this, std::placeholders::_1 );
}





MainWindow::~MainWindow()
{
    delete ui;
}




void MainWindow::main_v_play(QImage* img)
{
    QVideoWidget* video_widget = ui->widget;

    video_widget->videoSurface()->present(*img);
    //video_widget->show();
}




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




void MainWindow::set_signal_slot()
{
    connect( ui->pushButton, &QPushButton::clicked, this, &MainWindow::load_slot );
    connect( &worker, &Worker::recv_video_frame_signal, this, &MainWindow::recv_video_frame_slot );
}







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



