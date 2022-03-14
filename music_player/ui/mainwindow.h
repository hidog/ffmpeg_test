#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

#include "src/task_manager.h"
#include "ui/lockdialog.h"




QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE




class MusicWorker;
class PlayWorker;




class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void    closeEvent(QCloseEvent *event) override;

    void    set_connect();
    int     volume();

    MusicWorker*    get_music_worker();
    PlayWorker*     get_play_worker();

public slots:

    void    open_slot();
    void    task_finish_slot();
    void    play_slot( QString path );

private:
    Ui::MainWindow *ui;

    LockDialog      lock_dialog;
    TaskManager     task_manager;

    std::unique_ptr<MusicWorker>     music_worker;
    std::unique_ptr<PlayWorker>      play_worker;

};





#endif // MAINWINDOW_H
