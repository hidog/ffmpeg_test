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
class AllModel;
class FileModel;



enum class FinishBehavior
{
    NONE,
    STOP,
    NEXT,
    PREVIOUS,
    USER,
};





class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void    open( QString dir );
    void    copyto( QString src, QString dst );

    void    set_connect();
    int     volume();
    void    stop_for_user_click();
    bool    is_playing();
    bool    is_pause();
    void    refresh_current();
    void    set_finish_behavior( FinishBehavior fb );
    void    wait_worker_stop();
    void    wait_worker_start();
    void    clear_play_status();

    void    save_setting();
    void    load_setting();

    MusicWorker*    get_music_worker();
    PlayWorker*     get_play_worker();

    AllModel*       get_all_model();
    FileModel*      get_file_model();

    void    closeEvent(QCloseEvent *event) override;
    void    keyPressEvent(QKeyEvent *event) override;

public slots:

    void    open_slot();
    void    copyto_slot();
    void    task_finish_slot();
    void    play_button_slot();
    void    stop_button_slot();
    void    pause_button_slot();
    void    previous_button_slot();
    void    next_button_slot();
    void    repeat_button_slot();
    void    random_button_slot();
    void    favorite_button_slot();

    void    play_slot( QString path );
    void    duration_slot( int du );
    void    update_seekbar_slot( int sec );

    void    finish_slot();


private:
    Ui::MainWindow *ui;

    LockDialog      lock_dialog;
    TaskManager     task_manager;

    std::unique_ptr<MusicWorker>        music_worker;
    std::unique_ptr<PlayWorker>         play_worker;

    bool    repeat_flag     =   false;
    bool    random_flag     =   false;
    bool    favorite_flag   =   false;

    QString     root_path;

    int     total_time  =   0;
    QMetaObject::Connection     seek_connect[2];
    QMetaObject::Connection     finish_connect;

    FinishBehavior  finish_behavior     =   FinishBehavior::NONE;
};





#endif // MAINWINDOW_H
