#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "worker.h"



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void set_signal_slot();

    
    void audio_play();


    void main_v_play(QImage*);



public slots:

    void load_slot();
    void recv_video_frame_slot();



private:

    Ui::MainWindow *ui;

    Worker worker;

};
#endif // MAINWINDOW_H
