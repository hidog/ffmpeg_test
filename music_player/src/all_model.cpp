#include "all_model.h"

#include <QDebug>
#include <QFileIconProvider>
#include <QColor>




/*******************************************************************************
AllModel::AllModel()
********************************************************************************/
AllModel::AllModel( QObject *parent ) :
	QAbstractTableModel( parent )
{
    head_list << "*" << "*" << "*" << "*" << "name" << "duration" << "size" << "title" << "artist" << "album" << "full path";
	last_index	=	createIndex( 0, 0 );
}





/*******************************************************************************
AllModel::~AllModel()
********************************************************************************/
AllModel::~AllModel()
{}







/*******************************************************************************
AllModel::refresh_slot()
********************************************************************************/
void	AllModel::refresh_slot()
{}






/*******************************************************************************
AllModel::get_header_count()
********************************************************************************/
int		AllModel::get_header_count()
{
	return	head_list.size();
}






/*******************************************************************************
AllModel::rowCount()
********************************************************************************/
int		AllModel::rowCount( const QModelIndex &parent ) const 
{ 
	return	file_list.size();
}






/*******************************************************************************
AllModel::columnCount()
********************************************************************************/
int		AllModel::columnCount( const QModelIndex &parent ) const 
{ 
    return	head_list.size();
}







/*******************************************************************************
AllModel::refresh_singal()
********************************************************************************/
void	AllModel::refresh_singal( int row )
{
	int		col		=	head_list.size();

	QModelIndex		left_top		=	createIndex( row, 0 );
	QModelIndex		right_bottom	=	createIndex( row, col );

	emit dataChanged( left_top, right_bottom );
}







/*******************************************************************************
AllModel::refresh_view()
********************************************************************************/
void	AllModel::refresh_view()
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
AllModel::enter_dir_slot()
********************************************************************************/
void	AllModel::double_clicked_slot( const QModelIndex &index )
{
 	int			row		=	index.row();
	QFileInfo	info	=	file_list[row];
    emit play_signal(info.absoluteFilePath());    
}







/*******************************************************************************
AllModel::text_data()
********************************************************************************/
QVariant	AllModel::text_data( const QModelIndex &index, int role ) const
{
	int		col =   index.column();
	int		row =   index.row();

	QFileInfo	info    =   file_list[row];
	QVariant	result;

	// handle DisplayRole only.
	if( role != Qt::DisplayRole )
		assert(false);

    assert( info.isFile() == true );

	switch( col )
	{
		case 4:
			result	=	info.fileName();
			break;
		case 5:        
            result  =   QString("00:00:00");            
			break;		
		case 6:
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
			break;
        }
		case 7:
            // title
			break;
        case 8:
            // artist
            break;
        case 9:
            // album
            break;
        case 10:
			result	=	info.absoluteFilePath();
            break;
	}

	return	result;
}







/*******************************************************************************
AllModel::icon_data()
********************************************************************************/
QVariant	AllModel::icon_data( const QModelIndex &index, int role ) const
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

	switch( col )
	{
    case 0:
    	result	=	icon_pv.icon(info);
    	break;
	}

	return result;
}







/*******************************************************************************
AllModel::data()
********************************************************************************/
QVariant	AllModel::data( const QModelIndex &index, int role ) const
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
                var	=	QVariant(); //status_vec[row].color;*/
			break;
	}

	return var;
}






/*******************************************************************************
AllModel::get_font_color()
********************************************************************************/
QVariant	AllModel::get_font_color( const QModelIndex &index, int role ) const
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
AllModel::headerData()
********************************************************************************/
QVariant	AllModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if ( role == Qt::DisplayRole )
	{
		if ( orientation == Qt::Horizontal )
            return  head_list[section];
	}
	return QVariant();
}




/*******************************************************************************
AllModel::set_file_list()
********************************************************************************/
void    AllModel::set_file_list( QFileInfoList&& list )
{
    file_list   =   std::move(list);
	refresh_view();
}
