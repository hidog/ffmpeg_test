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

    // �������U�۩w�q������.
    qRegisterMetaType<VideoSetting>("VideoSetting");
    qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");


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

    // ���ӦҼ{�令timer�X��,��K�T�{�O�_��frame�Ӥ��θѥX��.
    // �����u����timer, �i�H��loop����
    // �p��C��frame�X�{���ɶ�, �W�L�N�qframe
    // seek���ɭ�, ��X�{�ɶ��k�s�᭫�s�p��. frame count ��u���b�v������frame count��_,�ӬO�C��seek����᭫�s�p��frame count.

    last_index = view_data->index;
    video_widget->videoSurface()->present( view_data->frame );

    video_mtx->unlock();
}




/*******************************************************************************
MainWindow::set_signal_slot()
********************************************************************************/
void MainWindow::set_signal_slot()
{
    // �¦��g�k
    connect(    worker,             SIGNAL(video_setting_signal(VideoSetting)),     this,           SLOT(set_video_setting_slot(VideoSetting))  );
    //connect(    worker,           &Worker::video_setting_singal,                  this,           &MainWindow::set_video_setting_slot         );
    connect(    worker,             &Worker::subtitle_list_signal,                  this,           &MainWindow::set_subtitle_list_slot         );
    connect(    worker,             &Worker::embedded_sublist_signal,               this,           &MainWindow::embedded_sublist_slot          );
    connect(    worker,             &Worker::finished,                              this,           &MainWindow::finish_slot                    );

    connect(    ui->subCBox,        &QComboBox::currentTextChanged,                 worker,         &Worker::switch_subtitle_slot_str           );
    connect(    ui->subCBox,        SIGNAL(currentIndexChanged(int)),               worker,         SLOT(switch_subtitle_slot_int(int))         );   // �Υt�@�Ӥ覡�|�����~
    connect(    ui->stopButton,     &QPushButton::clicked,                          worker,         &Worker::stop_slot                          );   
    connect(    ui->playButton,     &QPushButton::clicked,                          this,           &MainWindow::play_slot                      );
    connect(    ui->pauseButton,    &QPushButton::clicked,                          this,           &MainWindow::pause_slot                     );

    connect(    video_worker,       &VideoWorker::recv_video_frame_signal,          this,           &MainWindow::recv_video_frame_slot          );
    connect(    ui->volumeSlider,   &QSlider::valueChanged,                         audio_worker,   &AudioWorker::volume_slot                   );
}




/*******************************************************************************
MainWindow::set_video_setting_slot()
********************************************************************************/
void    MainWindow::set_video_setting_slot( VideoSetting vs )
{
    QVideoWidget    *video_widget   =   ui->videoWidget;

    QSize   size { vs.width, vs.height };

    QVideoSurfaceFormat     format { size, QVideoFrame::Format_RGB24 };
    video_widget->videoSurface()->start(format);   
    //video_widget->show();

    worker->finish_set_video();
}





/*******************************************************************************
MainWindow::play_slot()
********************************************************************************/
void MainWindow::play_slot()
{
    ui->subCBox->clear();

    QString filename     =   QFileDialog::getOpenFileName( this, tr("select src file"), "D:\\" );
    if( filename.isEmpty() == true )
        return;

    MYLOG( LOG::INFO, "load file %s", filename.toStdString().c_str() );
    worker->set_src_file(filename.toStdString());
   
    ui->playButton->setDisabled(true);
    ui->centralwidget->setFocus(); // ���[�o��|�y�� keyboard event ���h�@��

    worker->start();
}






/*******************************************************************************
MainWindow::pause_slot()
********************************************************************************/
void    MainWindow::pause_slot()
{
    pause();
}





/*******************************************************************************
MainWindow::pause()
********************************************************************************/
void    MainWindow::pause()
{
    video_worker->pause();
    audio_worker->pause();
}




/*******************************************************************************
MainWindow::finish_slot()
********************************************************************************/
void    MainWindow::finish_slot()
{
    ui->playButton->setEnabled(true);
    ui->videoWidget->videoSurface()->stop();
    ui->subCBox->clear();

    QVideoWidget    *video_widget   =   ui->videoWidget;
    video_widget->setFullScreen( false );
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
        {
            QVideoWidget    *video_widget   =   ui->videoWidget;
            bool    flag    =   video_widget->isFullScreen();
            video_widget->setFullScreen( !flag );
            break;
        }
        case Qt::Key_Space :
        {
            pause();
            break;
        }
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
MainWindow::embedded_sublist_slot()
********************************************************************************/
void    MainWindow::embedded_sublist_slot(  std::vector<std::string> list )
{
    int     i;
    auto    sub_combobox    =   ui->subCBox;

    for( i = 0; i < list.size(); i++ )
        sub_combobox->addItem( list.at(i).c_str() );
}




/*******************************************************************************
MainWindow::volume()
********************************************************************************/
int     MainWindow::volume()
{
    return  ui->volumeSlider->value();
}




/*******************************************************************************
MainWindow::closeEvent()
********************************************************************************/
void    MainWindow::closeEvent( QCloseEvent *event )
{
    worker->stop_slot();

    while( worker->isFinished() == false )
        SLEEP_10MS;
    while( video_worker->isFinished() == false )
        SLEEP_10MS;
    while( audio_worker->isFinished() == false )
        SLEEP_10MS;
}