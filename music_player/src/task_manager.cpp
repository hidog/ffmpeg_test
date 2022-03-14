#include "task_manager.h"

#include <QDir>
#include <QDebug>

#include "../../src/tool.h"



/*******************************************************************************
TaskManager::TaskManager()
********************************************************************************/
TaskManager::TaskManager()
{}





/*******************************************************************************
TaskManager::~TaskManager()
********************************************************************************/
TaskManager::~TaskManager()
{}






/*******************************************************************************
TaskManager::set_task_type()
********************************************************************************/
void    TaskManager::set_scan_task( QString path )
{
    type        =   TaskType::SCAN;
    scan_path   =   path;
}




/*******************************************************************************
TaskManager::set_task_type()
********************************************************************************/
void    TaskManager::run()
{
    stop_flag   =   false;

    switch( type )
    {
    case TaskType::SCAN:
        scan_count      =   0;
        last_pg_value   =   0;
        all_file_count  =   get_all_file_count(scan_path);
        scan_folder( scan_path );
        break;
    default:
        assert(0);
    }
}





/*******************************************************************************
TaskManager::scan_folder()
********************************************************************************/
void    TaskManager::scan_folder( QString path )
{
    SLEEP_1MS;

    QDir    dir(path);

    dir.setFilter( QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot );
    QFileInfoList   list    =   dir.entryInfoList();

    int         progress_value  =   0;
    QString     msg;

    msg     =   QString("scan %1").arg( dir.absolutePath() );
    emit message_singal(msg);

    for( auto& info : list )
    {
        if( stop_flag == true )
            break;

        //emit message_sig( QString("scan item %1").arg(info.fileName()) );
        //qDebug() << "path = " << info.absoluteFilePath();

        scan_count++;
        progress_value  =   100.0 * scan_count / all_file_count;
        if( progress_value > last_pg_value )
        {
            last_pg_value   =   progress_value;
            emit progress_signal(progress_value);
        }

        //scan_list.push_back(info);
        if( info.isDir() == true )
            scan_folder( info.absoluteFilePath() );
    }
}

   


/*******************************************************************************
TaskManager::get_all_file_count()
********************************************************************************/
int     TaskManager::get_all_file_count( QString path )
{
    QDir    dir(path);

    dir.setFilter( QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot );
    QFileInfoList   list    =   dir.entryInfoList();

    //
    int     count   =   dir.count();
    for( auto& info : list )    
        count   +=  get_all_file_count( info.absoluteFilePath() );    
    return  count;
}





/*******************************************************************************
TaskManager::cancel_slot()
********************************************************************************/
void    TaskManager::cancel_slot()
{
    stop_flag   =   true;
    this->quit();
}
