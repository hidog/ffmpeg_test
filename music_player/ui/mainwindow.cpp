#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QFileDialog>
#include <QDebug>

#include "../../src/tool.h"




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
    connect(    &task_manager,      &TaskManager::progress_signal,  &lock_dialog,   &LockDialog::progress_slot );  
    connect(    &task_manager,      &TaskManager::message_singal,  &lock_dialog,   &LockDialog::message_slot );  
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
    lock_dialog.hide();
}
