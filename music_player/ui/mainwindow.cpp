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

    connect(    ui->volumeSlider,     &QSlider::valueChanged,    music_worker.get(),   &MusicWorker::volume_slot  );

    connect(    &task_manager,      &QThread::finished,   this,   &MainWindow::task_finish_slot  );
    connect(    &task_manager,      &QThread::finished,   &lock_dialog,   &LockDialog::finish_slot  );
    connect(    &task_manager,      &TaskManager::progress_signal,  &lock_dialog,   &LockDialog::progress_slot );  
    connect(    &task_manager,      &TaskManager::message_singal,  &lock_dialog,   &LockDialog::message_slot );

    connect(    &lock_dialog,      &LockDialog::cancel_signal,  &task_manager,   &TaskManager::cancel_slot );

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
        return;
    }

    bool    flag    =   false;

    if( ui->tabWidget->currentIndex() == 0 )
    {
        AllWidget   *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
        assert( a_widget != nullptr );
        AllModel    *a_model    =   a_widget->get_model();

        if( random_flag == false )
            flag    =   a_model->play();
        else
            assert(0);
    }
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
    if( is_pause() == true )
        music_worker->pause();

    play_worker->stop_slot();
    QIcon   icon( QString("./img/play_1.png") );
    ui->playButton->setIcon(icon);

    wait_worker_stop();
    refresh_current();
}







/*******************************************************************************
MainWindow::previous_button_slot()
********************************************************************************/
void    MainWindow::previous_button_slot()
{
    if( is_pause() == true )
    {
        music_worker->pause();
        play_worker->stop_slot();
    }

    wait_worker_stop();

    bool    flag    =   false;

    if( ui->tabWidget->currentIndex() == 0 )
    {
        AllWidget   *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
        assert( a_widget != nullptr );
        AllModel    *a_model    =   a_widget->get_model();
        flag    =   a_model->previous();
    }
    else if( ui->tabWidget->currentIndex() == 1 )
    {}
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
    }
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
    if( is_pause() == true )
    {
        music_worker->pause();
        play_worker->stop_slot();
    }

    wait_worker_stop();

    bool    flag    =   false;

    if( ui->tabWidget->currentIndex() == 0 )
    {
        AllWidget   *a_widget   =   dynamic_cast<AllWidget*>(ui->tabWidget->widget(0));
        assert( a_widget != nullptr );
        AllModel    *a_model    =   a_widget->get_model();
        flag    =   a_model->next();
    }
    else if( ui->tabWidget->currentIndex() == 1 )
    {}
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
    }
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
    lock_dialog.close();

    play_worker->stop_slot();    
    wait_worker_stop();
}





/*******************************************************************************
MainWindow::wait_worker_stop()
********************************************************************************/
void    MainWindow::wait_worker_stop()
{
    int     count   =   0;

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