#include "file_model.h"

#include <QDebug>
#include <QFileIconProvider>
#include <QColor>

#include "mainwindow.h"
#include "all_model.h"




/*******************************************************************************
FileModel::FileModel()
********************************************************************************/
FileModel::FileModel( QObject *parent ) :
	QAbstractTableModel( parent )
{
    head_list << "*" << "N" << "F" << "*" << "name" << "duration" << "size" << "title" << "artist" << "album" << "full path";
}





/*******************************************************************************
FileModel::~FileModel()
********************************************************************************/
FileModel::~FileModel()
{}







/*******************************************************************************
FileModel::refresh_slot()
********************************************************************************/
void	FileModel::refresh_slot()
{}






/*******************************************************************************
FileModel::get_header_count()
********************************************************************************/
int		FileModel::get_header_count()
{
	return	head_list.size();
}






/*******************************************************************************
FileModel::rowCount()
********************************************************************************/
int		FileModel::rowCount( const QModelIndex &parent ) const 
{ 
	return	file_list.size();
}






/*******************************************************************************
FileModel::columnCount()
********************************************************************************/
int		FileModel::columnCount( const QModelIndex &parent ) const 
{ 
    return	head_list.size();
}








/*******************************************************************************
FileModel::refresh_singal()
********************************************************************************/
void	FileModel::refresh_singal( int row )
{
	int		col		=	head_list.size();

	QModelIndex		left_top		=	createIndex( row, 0 );
	QModelIndex		right_bottom	=	createIndex( row, col );

	emit dataChanged( left_top, right_bottom );
}







/*******************************************************************************
FileModel::refresh_view()

這邊會紀錄上一次的file_size
取max, 避免有網格沒更新到.
(例如從數量多的folder移動到數量少的)
P.S. 畫面更新沒測試過,有空在測試,包含效能的影響.
********************************************************************************/
void	FileModel::refresh_view()
{
	int		row		=	file_list.size() > last_index.row() ? file_list.size() : last_index.row();
	int		col		=	head_list.size();

	QModelIndex		left_top		=	createIndex( 0, 0 );
	QModelIndex		right_bottom	=	createIndex( row, col );

	emit dataChanged( left_top, right_bottom );
	emit refresh_signal();

	last_index	=	createIndex( file_list.size(), col );

}








/*******************************************************************************
FileModel::get_file_list()
********************************************************************************/
void	FileModel::get_file_list()
{
	// get relative path.
	QString		relative_path	=	QDir(root_path).relativeFilePath( dir.path() );  //dir.relativeFilePath( root_path );
	if( relative_path[0] == '.' )
		relative_path[0]	=	'/';
	else
		relative_path.push_front('/');

	if( *(relative_path.end()-1) != '/' )
		relative_path.push_back('/');

	emit path_change_signal( relative_path );

	//
    QStringList     name_filters;
    name_filters << "*.mp3" << "*.flac" << "";    

	QFileInfoList   list1    =   dir.entryInfoList( QDir::NoDot|QDir::Dirs, QDir::Name  );
	QFileInfoList   list2    =   dir.entryInfoList( name_filters, QDir::Files, QDir::Name  );

    file_list.clear();
    file_list.append(list1);
    file_list.append(list2);

	if( dir.path() == root_path )
		file_list.removeAt(0);
}







/*******************************************************************************
FileModel::enter_dir_slot()
********************************************************************************/
void	FileModel::double_clicked_slot( const QModelIndex &index )
{
 	int			row		=	index.row();
	QFileInfo	info	=	file_list[row];
	QString		path	=	info.fileName();

	if( info.isDir() )
	{
		if( path == QString("..") )
			dir.cdUp();
		else
			dir.cd(path);

		get_file_list();
		refresh_view();
	}
    else
    {
        QString   full_path     =   info.absoluteFilePath();
        all_model->play_by_path( full_path );
    }
}







/*******************************************************************************
FileModel::text_data()
********************************************************************************/
QVariant	FileModel::text_data( const QModelIndex &index, int role ) const
{
	int		col		=	index.column();
	int		row		=	index.row();

	QFileInfo	info		=	file_list[row];
	QVariant	result;

	// handle DisplayRole only.
	if( role != Qt::DisplayRole )
		assert(false);

	switch( col )
	{
		case 4:
			result	=	info.fileName();            
			break;
		case 5:
            if( info.isFile() == true )            
                result  =   QString("00:00:00");            
			break;		
		case 6:
            if( info.isFile() == true )
            {
                qint64      filesize    =   info.size();
                double      view_size   =   0;
                QString     unit_str    =   QString("bytes");
                if( filesize > 1048576 )  // 1024*1024
                {
                    view_size   =   1.0 * filesize / 1048576;
                    unit_str    =   "MB";
                }
                else if( filesize > 1024 )
                {
                    view_size  =   1.0 * filesize / 1024;
                    unit_str    =   "KB";
                }
                result  =   QString("%1 %2").arg(view_size, 0, 'f', 1 ).arg(unit_str);
            }
			break;
		case 7:
			if( info.isFile() == true )
            {
                // title
            }
			break;
        case 8:
            if( info.isFile() == true )
            {
                // artist
            }
            break;
        case 9:
            if( info.isFile() == true )
            {
                // album
            }
            break;
        case 10:
			result	=	info.absoluteFilePath();
            break;
	}

	return	result;
}







/*******************************************************************************
FileModel::icon_data()
********************************************************************************/
QVariant	FileModel::icon_data( const QModelIndex &index, int role ) const
{
	int		col		=	index.column();
	int		row		=	index.row();

	assert( row < file_list.size() );

	QFileInfo			info	=	file_list[row]; 
	QFileIconProvider	icon_pv;
	QVariant			result;

	// handle DecorationRole only.
	if( role != Qt::DecorationRole )
		assert(false);

    QIcon   play_icon(QString("./img/play_2.png"));

	switch( col )
	{
    case 0:
        if( info.isFile() )
        {
            QString     path    =   info.absoluteFilePath();
            bool        flag    =   all_model->is_now_play_by_path( path );
            if( flag == true )
                return  play_icon;
        }
        break;
    case 3:
    	result	=	icon_pv.icon(info);
    	break;
	}

	return result;
}







/*******************************************************************************
FileModel::data()
********************************************************************************/
QVariant	FileModel::data( const QModelIndex &index, int role ) const
{
	int		col		=	index.column();
	int		row		=	index.row();

	QVariant	var	=	QVariant();

	assert( row < file_list.size() );

	QFileInfo	info		=	file_list[row];

	switch( role )
	{
		case Qt::DisplayRole:
			var	=	text_data( index, role );
			break;
		case Qt::DecorationRole:
			var	=	icon_data( index, role );		
			break;
		/*case Qt::TextColorRole:
            if( col < 3 )
                var	=	QVariant(); //status_vec[row].color;
			break;*/
	}

	return var;
}






/*******************************************************************************
FileModel::get_font_color()
********************************************************************************/
QVariant	FileModel::get_font_color( const QModelIndex &index, int role ) const
{
	int		col		=	index.column();
	int		row		=	index.row();

	QFileInfo	info	=	file_list[row];
	QVariant	result;
	//GitStatus	git_status;

	// handle DisplayRole only.
	if( role != Qt::TextColorRole )
		assert(false);

	switch( col )
	{
		case 1:
		case 2:
			//result	=	git_status.get_file_color( dir.path(), info.fileName() );			
			break;		
	}

	return	result;
}






/*******************************************************************************
FileModel::set_root_path()
********************************************************************************/
void    FileModel::set_root_path( QString path )
{
    root_path   =   path;
    dir.setPath( root_path );
}






/*******************************************************************************
FileModel::init_file_list()
********************************************************************************/
void    FileModel::init_file_list()
{
	get_file_list();
	refresh_view();
}








/*******************************************************************************
FileModel::path_change_slot()
********************************************************************************/
void	FileModel::path_change_slot( const QString &new_path )
{
	dir.cd( root_path + new_path );
	get_file_list();
	refresh_view();
}








/*******************************************************************************
FileModel::headerData()
********************************************************************************/
QVariant	FileModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if ( role == Qt::DisplayRole )
	{
		if ( orientation == Qt::Horizontal )
            return  head_list[section];
	}
	return QVariant();
}






/*******************************************************************************
FileModel::set_mainwindow()
********************************************************************************/
void    FileModel::set_mainwindow( MainWindow *mw )
{
    main_window     =   mw;
}




/*******************************************************************************
FileModel::set_allmodel()
********************************************************************************/
void    FileModel::set_allmodel( AllModel* am )
{
    assert( all_model == nullptr );
    all_model   =   am;
}
