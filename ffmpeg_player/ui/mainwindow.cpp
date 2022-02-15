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

#include "player/player.h"
#include "worker.h"
#include "video_worker.h"
#include "audio_worker.h"

#ifdef _WIN32
#include <windows.h> // use for disable turn off monitor.
#else
#error need maintain.
#endif




/*******************************************************************************
MainWindow::MainWindow()
********************************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 必須註冊自定義的物件.
    qRegisterMetaType<VideoDecodeSetting>("VideoDecodeSetting");
    qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");

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

    VideoWidget    *video_widget   =   ui->videoWidget;

    static int  last_index  =   0;

    //
    video_mtx->lock();

    if( view_data->index - last_index != 1 )
        MYLOG( LOG::L_WARN, "vidw_data.index = %d, last_index = %d",  view_data->index , last_index );

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
    connect(    worker,             SIGNAL(video_setting_signal(VideoDecodeSetting)),     this,           SLOT(set_video_setting_slot(VideoDecodeSetting))  );

    //connect(    worker,           &Worker::video_setting_singal,                  this,           &MainWindow::set_video_setting_slot         );
    connect(    worker,             &Worker::subtitle_list_signal,                  this,           &MainWindow::set_subtitle_list_slot         );
    connect(    worker,             &Worker::embedded_sublist_signal,               this,           &MainWindow::embedded_sublist_slot          );
    connect(    worker,             &Worker::finished,                              this,           &MainWindow::finish_slot                    );
    connect(    worker,             &Worker::duration_signal,                       this,           &MainWindow::duration_slot                  );

    connect(    ui->subCBox,        &QComboBox::currentTextChanged,                 worker,         &Worker::switch_subtitle_slot_str           );
    connect(    ui->subCBox,        SIGNAL(currentIndexChanged(int)),               worker,         SLOT(switch_subtitle_slot_int(int))         );   // 用另一個方式會跳錯誤
    connect(    ui->stopButton,     &QPushButton::clicked,                          worker,         &Worker::stop_slot                          );   
    connect(    ui->playButton,     &QPushButton::clicked,                          this,           &MainWindow::play_slot                      );
    connect(    ui->pauseButton,    &QPushButton::clicked,                          this,           &MainWindow::pause_slot                     );
    connect(    ui->connectButton,  &QPushButton::clicked,                          this,           &MainWindow::start_connect_slot             );

    connect(    video_worker,       &VideoWorker::recv_video_frame_signal,          this,           &MainWindow::recv_video_frame_slot          );
    connect(    video_worker,       &VideoWorker::update_seekbar_signal,            this,           &MainWindow::update_seekbar_slot            );

    connect(    ui->volumeSlider,   &QSlider::valueChanged,                         audio_worker,   &AudioWorker::volume_slot                   );

}






/*******************************************************************************
MainWindow::start_connect_slot()
********************************************************************************/
void    MainWindow::start_connect_slot()
{
    worker->set_type( WorkType::SRT );
    ui->subCBox->clear();

    std::string     ip      =   ui->ipLEdit->text().toStdString();
    std::string     port    =   ui->portLEdit->text().toStdString();

    if( ip.empty() == true )
        ip      =   "127.0.0.1";
    if( port.empty() == true )
        port    =   "1234";

    worker->set_ip( ip );
    worker->set_port( port );
   
    ui->playButton->setDisabled(true);
    ui->centralwidget->setFocus();  // 不加這行會造成 keyboard event 失去作用

    worker->start();
}




/*******************************************************************************
MainWindow::set_video_setting_slot()
********************************************************************************/
void    MainWindow::set_video_setting_slot( VideoDecodeSetting vs )
{
    VideoWidget    *video_widget   =   ui->videoWidget;

    QSize   size { vs.width, vs.height };

    QVideoSurfaceFormat     format { size, QVideoFrame::Format_RGB24 };
    video_widget->videoSurface()->start(format);   
    //video_widget->show();

    worker->finish_set_video();
}






/*******************************************************************************
MainWindow::duration_slot()
********************************************************************************/
void    MainWindow::duration_slot( int du )
{
    total_time  =   du;
    ui->seekSlider->setMaximum(du);

    // 因為設置影片長度的時候會觸發 value 事件, 造成 lock. 
    // 暫時用設置完才 connect 跟 disconnect 的作法.
    seek_connect[0]    =   connect(    ui->seekSlider,     &QSlider::valueChanged,     worker,         &Worker::seek_slot            );
    seek_connect[1]    =   connect(    ui->seekSlider,     &QSlider::valueChanged,     audio_worker,   &AudioWorker::seek_slot       );
    seek_connect[2]    =   connect(    ui->seekSlider,     &QSlider::valueChanged,     video_worker,   &VideoWorker::seek_slot       );
}





/*******************************************************************************
MainWindow::update_seekbar()
********************************************************************************/
void    MainWindow::update_seekbar_slot( int sec )
{
    if( ui->seekSlider->is_mouse_press() == true )
        return;

    int     max     =   ui->seekSlider->maximum();
    int     min     =   ui->seekSlider->minimum();

    sec     =   sec > max ? max : sec;
    sec     =   sec < min ? min : sec;

    ui->seekSlider->setSliderPosition(sec);

    // update time 
    // 先放在這邊 未來有需求再把程式碼搬走
    int     s   =   sec % 60;
    int     m   =   sec / 60 % 60;
    int     h   =   sec / 60 / 60;

    int     ts  =   total_time % 60;
    int     tm  =   total_time / 60 % 60;
    int     th  =   total_time / 60 / 60;

    // 有空再來修這邊的排版
    QString     str =   QString("%1:%2:%3 / %4:%5:%6").arg(h).arg(m,2).arg(s,2).arg(th).arg(tm,2).arg(ts,2);
    ui->timeLabel->setText(str);
}




/*******************************************************************************
MainWindow::play_slot()
********************************************************************************/
void MainWindow::play_slot()
{
    std::string     port        =   ui->portLEdit->text().toStdString();
    bool            is_output   =   ui->outputCBox->checkState() == Qt::Checked;

    worker->set_type( WorkType::DEFAULT );
    worker->set_output( is_output, port );

    ui->subCBox->clear();

    QString filename     =   QFileDialog::getOpenFileName( this, tr("select src file"), "D:\\" );
    if( filename.isEmpty() == true )
        return;

    MYLOG( LOG::L_INFO, "load file %s", filename.toStdString().c_str() );
    worker->set_src_file(filename.toStdString());
   
    ui->playButton->setDisabled(true);
    ui->centralwidget->setFocus(); // 不加這行會造成 keyboard event 失去作用


    // 取消休眠
#ifdef _WIN32
    SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED );
#else
#error  undefined
#endif

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
    video_widget->setGeometry( QRect(70,70,1401,851) );  // 先寫死 之後改成能動態調整

    ui->seekSlider->setSliderPosition(0);
    ui->seekSlider->setValue(0);

    worker->end();

    disconnect( seek_connect[0] );
    disconnect( seek_connect[1] );
    disconnect( seek_connect[2] );

    // 恢復休眠
#ifdef _WIN32
    SetThreadExecutionState( ES_CONTINUOUS );
#else
#error undefined.
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

    while( worker->isRunning() == true )
        SLEEP_10MS;
    while( video_worker->isRunning() == true )
        SLEEP_10MS;
    while( audio_worker->isRunning() == true )
        SLEEP_10MS;
}