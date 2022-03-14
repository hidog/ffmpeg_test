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
        file_list.clear();
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

    QStringList     filters;
    filters << "*.mp3" << "*.flac";
    QFileInfoList   f_list    =   dir.entryInfoList( filters, QDir::Files|QDir::NoDotAndDotDot );
    QFileInfoList   d_list    =   dir.entryInfoList( QDir::Dirs|QDir::NoDotAndDotDot );

    int         progress_value  =   0;
    QString     msg;

    msg     =   QString("scan %1").arg( dir.absolutePath() );
    emit message_singal(msg);

    if( f_list.empty() == false )
        file_list.append( f_list );

    for( auto& info : d_list )
    {
        if( stop_flag == true )
            break;

        scan_count++;
        progress_value  =   100.0 * scan_count / all_file_count;
        if( progress_value > last_pg_value )
        {
            last_pg_value   =   progress_value;
            emit progress_signal(progress_value);
        }

        scan_folder( info.absoluteFilePath() );
    }

    if( stop_flag == true )    
        file_list.clear();
}

   


/*******************************************************************************
TaskManager::get_all_file_count()
********************************************************************************/
int     TaskManager::get_all_file_count( QString path )
{
    QDir    dir(path);
    QFileInfoList   d_list    =   dir.entryInfoList( QDir::Dirs|QDir::NoDotAndDotDot );    
    int     count   =   d_list.count();
    for( auto& info : d_list )    
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





/*******************************************************************************
TaskManager::get_file_list()
********************************************************************************/
QFileInfoList&&     TaskManager::get_file_list()
{
    return  std::move(file_list);
}
