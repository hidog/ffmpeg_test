#include "lockdialog.h"
#include "ui_lockdialog.h"

#include <QShowEvent>





/*******************************************************************************
LockDialog::LockDialog()
********************************************************************************/
LockDialog::LockDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lockdialog)
{
    ui->setupUi(this);

    set_connect();
}






/*******************************************************************************
LockDialog::~LockDialog()
********************************************************************************/
LockDialog::~LockDialog()
{
    delete ui;
}





/*******************************************************************************
LockDialog::progress_signal()
********************************************************************************/
void    LockDialog::progress_slot( int value )
{
    if( value < 0 )
        value   =   0;
    if( value > 100 )
        value   =   100;

    ui->progressBar->setValue(value);
}





/*******************************************************************************
LockDialog::progress_signal()
********************************************************************************/
void    LockDialog::set_task_name( QString name )
{
    ui->taskLabel->setText(name);
}





/*******************************************************************************
LockDialog::message_slot()
********************************************************************************/
void    LockDialog::message_slot( QString msg )
{
    ui->messageBrowser->setText( msg );
}






/*******************************************************************************
LockDialog::set_connect()
********************************************************************************/
void    LockDialog::set_connect()
{
    connect(    ui->cancelButton,      &QPushButton::clicked,  this,   &LockDialog::cancel_slot );
}





/*******************************************************************************
LockDialog::cancel_slot()
********************************************************************************/
void    LockDialog::cancel_slot()
{
    emit cancel_signal();
    ui->messageBrowser->clear();
    ui->progressBar->setValue(0);
}





/*******************************************************************************
LockDialog::cancel_slot()
********************************************************************************/
void    LockDialog::finish_slot()
{
    ui->messageBrowser->clear();
    ui->progressBar->setValue(0);
}


