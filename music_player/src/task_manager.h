#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <QThread>


enum class TaskType
{
    NONE    =   0,
    SCAN,
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

    void    set_scan_task( QString path );


signals:

    void    progress_signal( int value );
    void    message_singal( QString msg );

private:

    void    scan_folder( QString path );
    int     get_all_file_count( QString path );

    TaskType    type    =   TaskType::NONE;

    // for scan task
    QString     scan_path;
    int         all_file_count  =   0;
    int         scan_count      =   0;

};




#endif