#include "lockdialog.h"
#include "ui_lockdialog.h"

lockdialog::lockdialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lockdialog)
{
    ui->setupUi(this);
}

lockdialog::~lockdialog()
{
    delete ui;
}
