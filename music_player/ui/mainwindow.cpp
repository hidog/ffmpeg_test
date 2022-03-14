#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QFileDialog>
#include <QDebug>

#include "src/play_worker.h"
#include "src/music_worker.h"
#include "src/file_model.h"
#include "tool.h"




/*******************************************************************************
MainWindow::MainWindow()
********************************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    set_connect();
    lock_dialog.hide();

    music_worker    =   std::make_unique<MusicWorker>(this);
    play_worker     =   std::make_unique<PlayWorker>(this);
}





/*******************************************************************************
MainWindow::~MainWindow()
********************************************************************************/
MainWindow::~MainWindow()
{
    delete ui;
}




/*******************************************************************************
MainWindow::closeEvent()
********************************************************************************/
void    MainWindow::closeEvent( QCloseEvent *event ) 
{
    lock_dialog.close();
}





/*******************************************************************************
MainWindow::set_connect()
********************************************************************************/
void    MainWindow::set_connect()
{
    connect(    ui->openButton,     &QPushButton::clicked,    this,   &MainWindow::open_slot  );

    connect(    &task_manager,      &QThread::finished,   this,   &MainWindow::task_finish_slot  );
    connect(    &task_manager,      &QThread::finished,   &lock_dialog,   &LockDialog::finish_slot  );
    connect(    &task_manager,      &TaskManager::progress_signal,  &lock_dialog,   &LockDialog::progress_slot );  
    connect(    &task_manager,      &TaskManager::message_singal,  &lock_dialog,   &LockDialog::message_slot );

    connect(    &lock_dialog,      &LockDialog::cancel_signal,  &task_manager,   &TaskManager::cancel_slot );

    FileModel   *model  =   ui->fileWT->get_model();
    connect(    model,      &FileModel::play_signal,  this,   &MainWindow::play_slot );
}







/*******************************************************************************
MainWindow::set_connect()
********************************************************************************/
void    MainWindow::open_slot()
{
    QString     dir     =   QFileDialog::getExistingDirectory( this, tr("open music folder"), "D:\\", 
                                                               QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );

    qDebug() << dir;

    if( task_manager.isRunning() == true )
    {
        MYLOG( LOG::L_WARN, "task is running." );
        return;
    }

    if( dir.isEmpty() == true )
        return;
    
    // note: need stop play music.
    //lock_dialog.set_task_name( "scan" );
    //lock_dialog.show();

    //task_manager.set_scan_all_task(dir);
    //task_manager.start();

    ui->fileWT->set_root_path( dir );
}





/*******************************************************************************
MainWindow::task_finish_slot()
********************************************************************************/
void    MainWindow::task_finish_slot()
{
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
    play_worker->set_src_file( path.toStdString() );
    play_worker->start();
}
