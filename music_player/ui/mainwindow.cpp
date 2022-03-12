#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QFileDialog>
#include <QDebug>




/*******************************************************************************
MainWindow::MainWindow()
********************************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
}







/*******************************************************************************
MainWindow::set_connect()
********************************************************************************/
void    MainWindow::open_slot()
{
    QString     dir     =   QFileDialog::getExistingDirectory( this, tr("open music folder"), "D:\\", 
                                                               QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );

    qDebug() << dir;
}
