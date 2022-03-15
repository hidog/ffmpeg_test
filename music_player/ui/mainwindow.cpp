#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QFileDialog>
#include <QDebug>
#include <QTabWidget>
#include <QKeyEvent>

#include "src/play_worker.h"
#include "src/music_worker.h"
#include "src/file_model.h"
#include "src/all_model.h"
#include "ui/filewidget.h"
#include "src/myslider.h"
#include "tool.h"




/*******************************************************************************
MainWindow::MainWindow()
********************************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    lock_dialog.hide();

    music_worker    =   std::make_unique<MusicWorker>(this);
    play_worker     =   std::make_unique<PlayWorker>(this);

    AllWidget  *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
    assert( a_widget != nullptr );
    AllModel   *a_model      =   a_widget->get_model();
    a_model->set_mainwindow( this );

    set_connect();
}





/*******************************************************************************
MainWindow::~MainWindow()
********************************************************************************/
MainWindow::~MainWindow()
{
    delete ui;
}





/*******************************************************************************
MainWindow::finish_slot()
********************************************************************************/
void    MainWindow::finish_slot()
{
    ui->seekSlider->setSliderPosition(0);
    ui->seekSlider->setValue(0);

    play_worker->end();

    disconnect( seek_connect[0] );
    disconnect( seek_connect[1] );

    // continue play.
    bool    flag    =   false;

    AllWidget   *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
    assert( a_widget != nullptr );
    AllModel    *a_model    =   a_widget->get_model();

    if( random_flag == true && 
        finish_behavior != FinishBehavior::STOP && 
        finish_behavior != FinishBehavior::USER )
    {
        if( a_model->is_status_all_played() == false )
            flag    =   a_model->play_random( favorite_flag );
        else 
            flag    =   false;
    }
    else
    {
        if( ui->tabWidget->currentIndex() == 0 )
        {
            switch(finish_behavior)
            {
            case FinishBehavior::STOP:
                flag    =   false;
                break;
            case FinishBehavior::USER:
                flag    =   a_model->play_user();
                break;
            case FinishBehavior::NONE:
                flag    =   a_model->play_next();
                break;
            case FinishBehavior::NEXT:
                flag    =   a_model->next();
                break;
            case FinishBehavior::PREVIOUS:
                flag    =   a_model->previous();
                break;
            default:
                assert(0);
            }
        }
        else if( ui->tabWidget->currentIndex() == 1 )
            assert(0);
        else
            assert(0);
    }

    if( flag == true )
    {
        QIcon   icon( QString("./img/play_2.png") );
        ui->playButton->setIcon(icon);
        wait_worker_start();
    }
    else
    {
        QIcon   icon( QString("./img/play_1.png") );
        ui->playButton->setIcon(icon);
        a_model->refresh_current();
    }

    MYLOG( LOG::L_DEBUG, "finish finish_slot" );
}









/*******************************************************************************
MainWindow::set_connect()
********************************************************************************/
void    MainWindow::set_connect()
{
    connect(    ui->openButton,     &QPushButton::clicked,    this,   &MainWindow::open_slot  );
    connect(    ui->playButton,     &QPushButton::clicked,    this,   &MainWindow::play_button_slot  );    
    connect(    ui->stopButton,     &QPushButton::clicked,    this,   &MainWindow::stop_button_slot  );    
    connect(    ui->pauseButton,    &QPushButton::clicked,    this,   &MainWindow::pause_button_slot  );    
    connect(    ui->previousButton,    &QPushButton::clicked,    this,   &MainWindow::previous_button_slot  );  
    connect(    ui->nextButton,    &QPushButton::clicked,    this,   &MainWindow::next_button_slot  );
    connect(    ui->repeatButton,    &QPushButton::clicked,    this,   &MainWindow::repeat_button_slot  );
    connect(    ui->randomButton,    &QPushButton::clicked,    this,   &MainWindow::random_button_slot  );
    connect(    ui->favoriteButton,    &QPushButton::clicked,    this,   &MainWindow::favorite_button_slot  );

    connect(    play_worker.get(),      &PlayWorker::duration_signal,    this,           &MainWindow::duration_slot );


    finish_connect  =   connect(    play_worker.get(),      &QThread::finished,    this,           &MainWindow::finish_slot );


    connect(    ui->volumeSlider,     &QSlider::valueChanged,    music_worker.get(),   &MusicWorker::volume_slot  );

    connect(    &task_manager,      &QThread::finished,   this,   &MainWindow::task_finish_slot  );
    connect(    &task_manager,      &QThread::finished,   &lock_dialog,   &LockDialog::finish_slot  );
    connect(    &task_manager,      &TaskManager::progress_signal,  &lock_dialog,   &LockDialog::progress_slot );  
    connect(    &task_manager,      &TaskManager::message_singal,  &lock_dialog,   &LockDialog::message_slot );

    connect(    &lock_dialog,      &LockDialog::cancel_signal,  &task_manager,   &TaskManager::cancel_slot );

    connect(    music_worker.get(),       &MusicWorker::update_seekbar_signal,            this,           &MainWindow::update_seekbar_slot            );

    //
    FileWidget  *f_widget   =   dynamic_cast<FileWidget*>(ui->tabWidget->widget(1));
    assert( f_widget != nullptr );
    FileModel   *f_model      =   f_widget->get_model();
    connect(    f_model,      &FileModel::play_signal,  this,   &MainWindow::play_slot );

    //
    AllWidget  *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
    assert( a_widget != nullptr );
    AllModel   *a_model      =   a_widget->get_model();
    connect(    a_model,      &AllModel::play_signal,  this,   &MainWindow::play_slot );
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
    QLatin1Char qc('0');
    QString     str =   QString("%1:%2:%3 / %4:%5:%6").arg(h,2,10,qc).arg(m,2,10,qc).arg(s,2,10,qc).arg(th,2,10,qc).arg(tm,2,10,qc).arg(ts,2,10,qc);
    ui->timeLabel->setText(str);
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
    seek_connect[0]    =   connect(    ui->seekSlider,     &QSlider::valueChanged,     play_worker.get(),         &PlayWorker::seek_slot            );
    seek_connect[1]    =   connect(    ui->seekSlider,     &QSlider::valueChanged,     music_worker.get(),   &MusicWorker::seek_slot       );
}






/*******************************************************************************
MainWindow::set_connect()
********************************************************************************/
void    MainWindow::open_slot()
{
    QString     dir     =   QFileDialog::getExistingDirectory( this, tr("open music folder"), "D:\\", 
                                                               QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );

    root_path   =   dir;

    if( task_manager.isRunning() == true )
    {
        MYLOG( LOG::L_WARN, "task is running." );
        return;
    }

    if( dir.isEmpty() == true )
        return;
    
    // note: need stop play music.
    lock_dialog.set_task_name( "scan" );
    lock_dialog.show();

    task_manager.set_scan_task(dir);
    task_manager.start();   
}





/*******************************************************************************
MainWindow::task_finish_slot()
********************************************************************************/
void    MainWindow::task_finish_slot()
{
    QFileInfoList   list    =   task_manager.get_file_list();
    if( list.empty() == true )  // 可能是取消掃描.
    {
        lock_dialog.hide();
        return;
    }

    AllWidget  *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
    assert( a_widget != nullptr );
    a_widget->set_file_list( std::move(list) );

    //
    FileWidget  *f_widget   =   dynamic_cast<FileWidget*>(ui->tabWidget->widget(1));
    assert( f_widget != nullptr );
    f_widget->set_root_path( root_path );

    lock_dialog.hide();
}





/*******************************************************************************
MainWindow::get_music_worker()
********************************************************************************/
MusicWorker*    MainWindow::get_music_worker()
{
    return  music_worker.get();
}





/*******************************************************************************
MainWindow::volume()
********************************************************************************/
int     MainWindow::volume()
{
    return  ui->volumeSlider->value();
}






/*******************************************************************************
MainWindow::get_play_worker()
********************************************************************************/
PlayWorker*     MainWindow::get_play_worker()
{
    return  play_worker.get();
}





/*******************************************************************************
MainWindow::play_slot()
********************************************************************************/
void    MainWindow::play_slot( QString path )
{
    //is_stop_flag    =   false;

    if( play_worker->isRunning() == true )
        play_worker->stop_slot();    

    wait_worker_stop();
    
    play_worker->set_src_file( path.toStdString() );
    play_worker->start();    
}






/*******************************************************************************
MainWindow::play_button_slot()
********************************************************************************/
void    MainWindow::play_button_slot()
{
    if( is_pause() == true )
    {
        music_worker->pause();
        refresh_current();
        return;
    }

    if( is_playing() == true )
        return;

    finish_behavior =   FinishBehavior::NONE;
    bool    flag    =   false;

    AllWidget   *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
    assert( a_widget != nullptr );
    AllModel    *a_model    =   a_widget->get_model();

    if( ui->tabWidget->currentIndex() == 0 )
        flag    =   a_model->play( random_flag, favorite_flag );
    else if( ui->tabWidget->currentIndex() == 1 )
        assert(0);
    else
        assert(0);

    if( flag == true )
    {
        QIcon   icon( QString("./img/play_2.png") );
        ui->playButton->setIcon(icon);
    }
    else
    {
        QIcon   icon( QString("./img/play_1.png") );
        ui->playButton->setIcon(icon);
        music_worker->close_io();
    }
}





/*******************************************************************************
MainWindow::is_playing()
********************************************************************************/
bool    MainWindow::is_playing()
{
    return  play_worker->isRunning();
}




/*******************************************************************************
MainWindow::stop_button_slot()
********************************************************************************/
void    MainWindow::stop_button_slot()
{
    if( is_playing() == false )
        return;

    if( is_pause() == true )
        music_worker->pause();

    finish_behavior =   FinishBehavior::STOP;

    play_worker->stop_slot();
    QIcon   icon( QString("./img/play_1.png") );
    ui->playButton->setIcon(icon);

    wait_worker_stop();
    refresh_current();
    music_worker->close_io();
}




/*******************************************************************************
MainWindow::stop()
********************************************************************************/
void    MainWindow::stop_for_user_click()
{
    assert( is_playing() == true );

    if( is_pause() == true )
        music_worker->pause();

    play_worker->stop_slot();
    QIcon   icon( QString("./img/play_1.png") );
    ui->playButton->setIcon(icon);
}





/*******************************************************************************
MainWindow::previous_button_slot()
********************************************************************************/
void    MainWindow::previous_button_slot()
{
    if( is_playing() == false )
        return;
    if( play_worker->get_stop_flag() == true )
        return;

    finish_behavior =   FinishBehavior::PREVIOUS;

    if( is_pause() == true )    
        music_worker->pause();
    
    play_worker->stop_slot();   
    wait_worker_stop();
}






/*******************************************************************************
MainWindow::repeat_button_slot()
********************************************************************************/
void    MainWindow::repeat_button_slot()
{
    QIcon   ic1( QString("./img/repeat.png") );
    QIcon   ic2( QString("./img/repeat_s.png") );

    repeat_flag     =   !repeat_flag;
    if( repeat_flag == true )
        ui->repeatButton->setIcon(ic2);
    else
        ui->repeatButton->setIcon(ic1);

    clear_play_status();
}






/*******************************************************************************
MainWindow::clear_play_status()
********************************************************************************/
void    MainWindow::clear_play_status()
{
    AllWidget   *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
    assert( a_widget != nullptr );
    AllModel    *a_model    =   a_widget->get_model();
    a_model->clear_played_state();
}







/*******************************************************************************
MainWindow::random_button_slot()
********************************************************************************/
void    MainWindow::random_button_slot()
{
    QIcon   ic1( QString("./img/random.png") );
    QIcon   ic2( QString("./img/random_s.png") );

    random_flag     =   !random_flag;
    if( random_flag == true )
        ui->randomButton->setIcon(ic2);
    else
        ui->randomButton->setIcon(ic1);

    clear_play_status();
}





/*******************************************************************************
MainWindow::favorite_button_slot()
********************************************************************************/
void    MainWindow::favorite_button_slot()
{
    QIcon   ic1( QString("./img/favorite.png") );
    QIcon   ic2( QString("./img/favorite_s.png") );

    favorite_flag     =   !favorite_flag;
    if( favorite_flag == true )
        ui->favoriteButton->setIcon(ic2);
    else
        ui->favoriteButton->setIcon(ic1);
}






/*******************************************************************************
MainWindow::next_button_slot()
********************************************************************************/
void    MainWindow::next_button_slot()
{
    if( is_playing() == false )
        return;
    if( play_worker->get_stop_flag() == true )
        return;

    finish_behavior =   FinishBehavior::NEXT;

    if( is_pause() == true )    
        music_worker->pause();
    
    play_worker->stop_slot();
    wait_worker_stop();
}





/*******************************************************************************
MainWindow::pause_button_slot()
********************************************************************************/
void    MainWindow::pause_button_slot()
{
    music_worker->pause();
    refresh_current();
}




/*******************************************************************************
MainWindow::set_finish_behavior()
********************************************************************************/
void    MainWindow::set_finish_behavior( FinishBehavior fb )
{
    finish_behavior =   fb;
}





/*******************************************************************************
MainWindow::pause_button_slot()
********************************************************************************/
void    MainWindow::refresh_current()
{
    AllWidget  *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
    assert( a_widget != nullptr );
    AllModel   *a_model      =   a_widget->get_model();
    a_model->refresh_current();
}





/*******************************************************************************
MainWindow::is_pause()
********************************************************************************/
bool    MainWindow::is_pause()
{
    return  music_worker->is_pause();
}






/*******************************************************************************
MainWindow::closeEvent()
********************************************************************************/
void    MainWindow::closeEvent( QCloseEvent *event )
{
    disconnect( finish_connect );
    finish_behavior =   FinishBehavior::STOP;
    lock_dialog.close();
    play_worker->stop_slot();    
    wait_worker_stop();
}





/*******************************************************************************
MainWindow::wait_for_start()
********************************************************************************/
void    MainWindow::wait_worker_start()
{
    while( play_worker->isRunning() == false )
        SLEEP_10MS;
    while( music_worker->isRunning() == false )
        SLEEP_10MS;
}





/*******************************************************************************
MainWindow::wait_worker_stop()
********************************************************************************/
void    MainWindow::wait_worker_stop()
{
    bool    fw_flag     =   play_worker->wait( 3000 );
    if( fw_flag == false )
        MYLOG( LOG::L_ERROR, "time out.");

    bool    mw_flag     =   music_worker->wait( 3000 );
    if( mw_flag == false )
        MYLOG( LOG::L_ERROR, "time out.");
}




/*******************************************************************************
MainWindow::keyPressEvent()
********************************************************************************/
void    MainWindow::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
        case Qt::Key_Space :
        {
            //pause();
            break;
        }
    }
}