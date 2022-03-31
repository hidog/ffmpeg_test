#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <QThread>
#include <QFileInfo>

#include "all_model.h"



enum class TaskType
{
    NONE    =   0,
    SCAN,
    COPYTO,
};




class TaskManager : public QThread
{
    Q_OBJECT

public:
    TaskManager();
    ~TaskManager();

    TaskManager( const TaskManager& ) = delete;
    TaskManager( TaskManager&& ) = delete;

    TaskManager& operator = ( const TaskManager& ) = delete;
    TaskManager& operator = ( TaskManager&& ) = delete;

    void    run() override;

    //
    void    set_scan_task( QString path );
    QFileInfoList&&     get_file_list();

    //
    void    set_copyto_task( QString src, QString dst, QVector<QFileInfo> fv, QVector<PlayStatus> pv );
    void    copyto();


public slots:

    void    cancel_slot();

signals:

    void    progress_signal( int value );
    void    message_singal( QString msg );

private:

    void    scan_folder( QString path );
    int     get_all_file_count( QString path );

    TaskType    type    =   TaskType::NONE;
    
    bool    stop_flag   =   false;

    // for scan task
    QString     scan_path;
    int         all_file_count  =   0;
    int         scan_count      =   0;
    int         last_pg_value   =   0;

    QFileInfoList   file_list;

    // for copyto task
    QString     src_path, dst_path; 
    QVector<QFileInfo>  copy_src_vec;
    QVector<PlayStatus> status_vec;

};




#endif