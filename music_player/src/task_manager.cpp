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
    case TaskType::COPYTO:        
        copyto();
        break;
    default:
        assert(0);
    }
}



/*******************************************************************************
TaskManager::set_task_type()
********************************************************************************/
void    TaskManager::copyto()
{
    int     progress_value  =   0;
    int     size    =   copy_src_vec.size();
    QString msg;


    for( int i = 0; i < size; i++ )
    {
        if( stop_flag == true )
        {
            MYLOG( LOG::L_WARN, "user interrupt" );
            break;
        }

        progress_value  =   100.0 * i / size;
        emit progress_signal(progress_value);

        if( status_vec[i].is_favorite == false )
            continue;

        QString     from_path   =   copy_src_vec[i].absoluteFilePath();
        QString     target      =   copy_src_vec[i].absoluteFilePath();
        target.remove(src_path);

        QStringList     folders =   target.split( QLatin1Char('/') );
        folders.removeLast();  // last is file.

        QDir    dir(dst_path);

        for( auto itr : folders )
        {
            if( dir.exists(itr) == false )
                dir.mkdir(itr);
            dir.cd(itr);            
        }

        QString     to_path =   QString("%1%2").arg(dst_path).arg(target);
        msg     =   QString("copy %1 to %2").arg(from_path).arg(to_path);
        emit message_singal(msg);

        QFile       file(from_path);
        file.copy(to_path);

        SLEEP_1US;
    }

    MYLOG( LOG::L_INFO, "copy finish." );
}





/*******************************************************************************
TaskManager::scan_folder()
********************************************************************************/
void    TaskManager::scan_folder( QString path )
{
    SLEEP_1US;

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
    {
        if( stop_flag == true )
            break;
        count   +=  get_all_file_count( info.absoluteFilePath() );
    }
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





/*******************************************************************************
TaskManager::set_copyto_path()
********************************************************************************/
void    TaskManager::set_copyto_task( QString src, QString dst, QVector<QFileInfo> fv, QVector<PlayStatus> pv )
{
    type        =   TaskType::COPYTO;
    
    src_path    =   src;
    if( src_path[src_path.size()-1] != QLatin1Char('/') )
        src_path.append( QLatin1Char('/') );
    
    dst_path    =   dst;
    if( dst_path[dst_path.size()-1] != QLatin1Char('/') )
        dst_path.append( QLatin1Char('/') );

    copy_src_vec    =   std::move(fv);
    status_vec      =   std::move(pv);
}
