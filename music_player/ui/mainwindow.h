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
class QCloseEvent;
class QKeyEvent;



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void    set_connect();
    int     volume();
    bool    is_playing();
    bool    is_pause();
    void    refresh_current();

    MusicWorker*    get_music_worker();
    PlayWorker*     get_play_worker();

    void    closeEvent(QCloseEvent *event) override;
    void    keyPressEvent(QKeyEvent *event) override;

    void    wait_worker_stop();

public slots:

    void    open_slot();
    void    task_finish_slot();
    void    play_button_slot();
    void    stop_button_slot();
    void    pause_button_slot();
    void    previous_button_slot();

    void    play_slot( QString path );

private:
    Ui::MainWindow *ui;

    LockDialog      lock_dialog;
    TaskManager     task_manager;

    std::unique_ptr<MusicWorker>        music_worker;
    std::unique_ptr<PlayWorker>         play_worker;

    QString     root_path;

};





#endif // MAINWINDOW_H
