#include "lockdialog.h"
#include "ui_lockdialog.h"





/*******************************************************************************
LockDialog::LockDialog()
********************************************************************************/
LockDialog::LockDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lockdialog)
{
    ui->setupUi(this);
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
    ui->messageBrowser->append( msg );
}
