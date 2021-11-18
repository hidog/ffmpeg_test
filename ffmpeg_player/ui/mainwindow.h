#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tool.h"




QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class Worker;
class VideoWorker;
class AudioWorker;
class VideoData;
class QMutex;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void set_signal_slot();

    
    void audio_play();


    void main_v_play(QImage*);


    QMutex*     get_video_mutex();
    VideoData*  get_view_data();


public slots:

    void load_slot();
    void recv_video_frame_slot();



private:

    Ui::MainWindow *ui;

    Worker          *worker         =   nullptr;  // 未來改成 unique pointer
    VideoWorker     *video_worker   =   nullptr;
    AudioWorker     *audio_worker   =   nullptr;

    QMutex          *video_mtx      =   nullptr;

    VideoData       *view_data      =   nullptr;

};
#endif // MAINWINDOW_H
