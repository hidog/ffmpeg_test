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
void	FileModel::refresh_row( int row )
{
	int		col		=	head_list.size();

	QModelIndex		left_top		=	createIndex( row, 0 );
	QModelIndex		right_bottom	=	createIndex( row, col );

	emit dataChanged( left_top, right_bottom );
}







/*******************************************************************************
FileModel::refresh_view()

�o��|�����W�@����file_size
��max, �קK������S��s��.
(�Ҧp�q�ƶq�h��folder���ʨ�ƶq�֪�)
P.S. �e����s�S���չL,���Ŧb����,�]�t�į઺�v�T.
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

	QFileInfoList   list1    =   dir.entryInfoList( QDir::NoDot|QDir::Dirs, QDir::Name );
	QFileInfoList   list2    =   dir.entryInfoList( name_filters, QDir::Files, QDir::Name );

    if( dir.path() == root_path )
		list1.removeAt(0);  

    file_list.clear();
    file_list.append(list1);
    file_list.append(list2);

    file_start_index    =   list1.size();
    update_status_vec( list2 ); 
}





/*******************************************************************************
FileModel::update_status_vec()
********************************************************************************/
void    FileModel::update_status_vec( const QFileInfoList& list )
{
    if( list.isEmpty() == true )
    {
        status_vec.clear();
        return;
    }
    status_vec  =   all_model->get_status_vec( list );
    qDebug() << status_vec.size();
}





/*******************************************************************************
FileModel::update_vec_slot()
********************************************************************************/
void    FileModel::update_vec_slot()
{
    QStringList     name_filters;
    name_filters << "*.mp3" << "*.flac" << "";    

	QFileInfoList   list     =   dir.entryInfoList( name_filters, QDir::Files, QDir::Name  );
    update_status_vec( list ); 
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
        {
            // �n��s tab widget select����.
            QString cur_path    =   dir.dirName();

			dir.cdUp();

            QStringList cd_list =   dir.entryList(QDir::NoDot|QDir::Dirs, QDir::Name);
            if( dir.path() == root_path )
                cd_list.removeAt(0);

            cd_select_index     =   cd_list.indexOf(cur_path);
            if( cd_select_index < 0 )
                cd_select_index =   0;
        }
		else
			dir.cd(path);

		get_file_list();
		refresh_view();
	}
    else
    {
        QString   full_path     =   info.absoluteFilePath();
        all_model->play_by_path( full_path );
        refresh_row( row );
    }
}




/*******************************************************************************
FileModel::clicked_slot()
********************************************************************************/
void    FileModel::clicked_slot( const QModelIndex &index )
{
    int     row     =   index.row();
    int     col     =   index.column();
    int     idx     =   row - file_start_index;
    if( idx < 0 )
        return;

    if( col == 2 )
    {
        QString     path    =   file_list[row].absoluteFilePath();
        status_vec[idx].is_favorite =  !status_vec[idx].is_favorite;
        all_model->set_favorite( path, status_vec[idx].is_favorite );
        refresh_row( row );
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
            if( info.fileName() == QString("..") )
                result  =   QString::fromLocal8Bit("�^�W�@�h");
            else
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
    int     idx     =   row - file_start_index;

	QFileInfo			info	=	file_list[row]; 
	QFileIconProvider	icon_pv;
	QVariant			result;

	// handle DecorationRole only.
	if( role != Qt::DecorationRole )
		assert(false);

    static QIcon   play_icon(QString("./img/play_2.png"));
    static QIcon   fvt_e_icon(QString("./img/favorite_e.png"));
    static QIcon   fvt_s_icon(QString("./img/favorite_s.png"));
    static QIcon   new_icon(QString("./img/new.png"));

	switch( col )
	{
    case 0:
        if( info.isFile() == true && main_window->is_playing() == true )
        {
            QString     path    =   info.absoluteFilePath();
            bool        flag    =   all_model->is_now_play_by_path( path );
            if( flag == true )
                return  play_icon;
        }
        break;
    case 1:
        if( info.isFile() == true )
        {
            if( status_vec[idx].is_new == true )        
                result  =   new_icon;
        }
        break;
    case 2:
        if( info.isFile() == true )
        {
            if( status_vec[idx].is_favorite == true )
                result  =   fvt_s_icon;
            else
                result  =   fvt_e_icon;
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
        case Qt::BackgroundColorRole:
        {
            QString     path    =   info.absoluteFilePath();
            if( all_model->is_now_play_by_path(path) && main_window->is_playing() == true )
                var     =   QColor( 0, 0, 255, 30 );
        }
            break;
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




/*******************************************************************************
FileModel::jump_slot()
********************************************************************************/
void    FileModel::jump_slot()
{
    if( main_window->is_playing() == false )
        return;

    const QFileInfo&    current_fileinfo    =   all_model->get_current_play_file();
    QString     target_path     =   current_fileinfo.absolutePath();

    if( target_path == dir.absolutePath() )
        return;

    dir.cd(target_path);
	get_file_list();
    refresh_view();
}





/*******************************************************************************
FileModel::get_cd_index()
********************************************************************************/
int     FileModel::get_cd_index()
{
    int     tmp     =   cd_select_index;
    cd_select_index =   0;
    return  tmp;
}
