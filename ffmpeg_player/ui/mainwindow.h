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
class QVideoWidget;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void    set_signal_slot();

    //
    QMutex*     get_video_mutex();
    VideoData*  get_view_data();

    //
    Worker*         get_worker();
    VideoWorker*    get_video_worker();
    AudioWorker*    get_audio_worker();

    void    keyPressEvent(QKeyEvent *event) override;


public slots:

    void    load_file_slot();
    void    start_slot();
    void    recv_video_frame_slot();
    void    set_video_setting_slot( VideoSetting vs );
    void    set_subtitle_list_slot( QStringList list );
    void    embedded_sublist_slot( std::vector<std::string> );

private:

    Ui::MainWindow *ui;

    //QVideoWidget    *video_widget; 可以用這個方式產生 video 視窗, 但是會遇到直接關掉此視窗,造成關閉軟體的時候會crash. 在研究怎麼修


    Worker          *worker         =   nullptr;  // 未來改成 unique pointer
    VideoWorker     *video_worker   =   nullptr;
    AudioWorker     *audio_worker   =   nullptr;

    QMutex          *video_mtx      =   nullptr;
    VideoData       *view_data      =   nullptr;

};
#endif // MAINWINDOW_H
