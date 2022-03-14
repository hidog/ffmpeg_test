#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "src/task_manager.h"
#include "ui/lockdialog.h"




QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE





class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void    closeEvent(QCloseEvent *event) override;

    void    set_connect();

public slots:

    void    open_slot();
    void    task_finish_slot();

private:
    Ui::MainWindow *ui;

    LockDialog      lock_dialog;
    TaskManager     task_manager;

};





#endif // MAINWINDOW_H
