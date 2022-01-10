#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tool.h"
#include "player/play_def.h"




QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class   Worker;
class   VideoWorker;
class   AudioWorker;
struct  VideoData;
class   QMutex;
class   QVideoWidget;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void    set_signal_slot();
    int     volume();
    void    pause();

    QMutex*     get_video_mutex();
    VideoData*  get_view_data();
    
    Worker*         get_worker();
    VideoWorker*    get_video_worker();
    AudioWorker*    get_audio_worker();

    void    keyPressEvent(QKeyEvent *event) override;
    void    closeEvent(QCloseEvent *event) override;

public slots:

    void    play_slot();
    void    pause_slot();

    void    recv_video_frame_slot();
    void    set_video_setting_slot( VideoDecodeSetting vs );
    void    set_subtitle_list_slot( QStringList list );
    void    embedded_sublist_slot( std::vector<std::string> );
    void    finish_slot();

    void    duration_slot( int du );
    void    update_seekbar_slot( int sec );

private: 
    Ui::MainWindow *ui;

    //QVideoWidget    *video_widget; �i�H�γo�Ӥ覡���� video ����, ���O�|�J�쪽������������,�y�������n�骺�ɭԷ|crash. �b��s����

    Worker          *worker         =   nullptr;  // ���ӧ令 unique pointer
    VideoWorker     *video_worker   =   nullptr;
    AudioWorker     *audio_worker   =   nullptr;

    QMutex          *video_mtx      =   nullptr;
    VideoData       *view_data      =   nullptr;

    QMetaObject::Connection     seek_connect[3];

    int     total_time  =   0;

};
#endif // MAINWINDOW_H
