#include "all_model.h"

#include <random>

#include <QDebug>
#include <QFileIconProvider>
#include <QColor>

#include "../ui/mainwindow.h"
#include "../../src/tool.h"




/*******************************************************************************
AllModel::AllModel()
********************************************************************************/
AllModel::AllModel( QObject *parent ) :
	QAbstractTableModel( parent )
{
    head_list << "*" << "N" << "F" << "*" << "name" << "duration" << "size" << "title" << "artist" << "album" << "full path";
               // play  new  favorite  icon
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
	return	file_vec.size();
}






/*******************************************************************************
AllModel::columnCount()
********************************************************************************/
int		AllModel::columnCount( const QModelIndex &parent ) const 
{ 
    return	head_list.size();
}






/*******************************************************************************
AllModel::refresh_current()
********************************************************************************/
void    AllModel::refresh_index( int col, int row )
{
    QModelIndex		left_top		=	createIndex( col, row );
	QModelIndex		right_bottom	=	createIndex( col, row );

	emit dataChanged( left_top, right_bottom );
}




/*******************************************************************************
AllModel::refresh_current()
********************************************************************************/
void    AllModel::refresh_current()
{
    int		col		=	head_list.size();

	QModelIndex		left_top		=	createIndex( play_index, 0 );
	QModelIndex		right_bottom	=	createIndex( play_index, col );
	emit dataChanged( left_top, right_bottom );
    emit show_row_signal( play_index );
}






/*******************************************************************************
AllModel::refresh_view()
********************************************************************************/
void	AllModel::refresh_list()
{
	int		row		=	file_vec.size();
	int		col		=	head_list.size();

	QModelIndex		left_top		=	createIndex( 0, 0 );
	QModelIndex		right_bottom	=	createIndex( row, col );

	emit dataChanged( left_top, right_bottom );
	emit refresh_signal();
}






/*******************************************************************************
AllModel::double_clicked_slot()
********************************************************************************/
void	AllModel::double_clicked_slot( const QModelIndex &index )
{
 	int			row		=	index.row();
	QFileInfo	info	=	file_vec[row];

    if( main_window->is_playing() == false )
    {
        main_window->set_finish_behavior( FinishBehavior::NONE );
        play_index  =   row;
        emit play_signal(info.absoluteFilePath());
        refresh_current();
    }
    else
    {
        main_window->stop_for_user_click();
        main_window->set_finish_behavior( FinishBehavior::USER );
        play_index  =   row;    
    }
}




/*******************************************************************************
AllModel::clicked_slot()
********************************************************************************/
void	AllModel::clicked_slot( const QModelIndex &index )
{
 	int     row		=	index.row();
    int     col     =   index.column();

    if( col == 2 )
    {
        status_vec[row].is_favorite =  !status_vec[row].is_favorite;
        refresh_index( col, row );
    }

    emit update_status_signal();
}



/*******************************************************************************
AllModel::set_favorite()
********************************************************************************/
void    AllModel::set_favorite( QString path, bool flag )
{
    int     i;
    for( i = 0; i < file_vec.size(); i++ )
    {
        if( file_vec[i].absoluteFilePath() == path )
            break;
    }

    if( i >= file_vec.size() )
    {
        MYLOG( LOG::L_WARN, "not found." );
        return;
    }

    status_vec[i].is_favorite   =   flag;
    refresh_index( 2, i );
}






/*******************************************************************************
AllModel::play_user()
********************************************************************************/
bool    AllModel::play_user()
{
    clear_played_state();
    assert( play_index < file_vec.size() && play_index >= 0 );       
	QFileInfo	info	=	file_vec[play_index];
    assert( main_window->is_playing() == false );
    main_window->set_finish_behavior( FinishBehavior::NONE );
    emit play_signal(info.absoluteFilePath());
    refresh_current();
    return  true;
}




/*******************************************************************************
AllModel::text_data()
********************************************************************************/
QVariant	AllModel::text_data( const QModelIndex &index, int role ) const
{
	int		col =   index.column();
	int		row =   index.row();

	const QFileInfo&	info    =   file_vec[row];
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
    // handle DecorationRole only.
	if( role != Qt::DecorationRole )
		assert(false);

	int		col		=	index.column();
	int		row		=	index.row();

	assert( row < file_vec.size() );

	const QFileInfo&	info	=	file_vec[row]; 
	QFileIconProvider	icon_pv;
	QVariant			result;

    static QIcon   play_icon(QString("./img/play_2.png"));
    static QIcon   pause_icon(QString("./img/pause.png"));
    static QIcon   new_icon(QString("./img/new.png"));
    static QIcon   fvt_e_icon(QString("./img/favorite_e.png"));
    static QIcon   fvt_s_icon(QString("./img/favorite_s.png"));

	switch( col )
	{
    case 0:
        if( play_index == row )
        {
            if( main_window->is_playing() == true )
                result  =   play_icon;
            if( main_window->is_pause() == true )
                result  =   pause_icon;
        }
        break;
    case 1:
        if( status_vec[row].is_new == true )        
            result  =   new_icon;
        break;
    case 2:
        if( status_vec[row].is_favorite == true )
            result  =   fvt_s_icon;
        else
            result  =   fvt_e_icon;
        break;
    case 3:
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

	assert( row < file_vec.size() );

	const QFileInfo&    info    =	file_vec[row];

	switch( role )
	{
		case Qt::DisplayRole:
			var	=	text_data( index, role );
			break;
		case Qt::DecorationRole:
			var	=	icon_data( index, role );		
			break;
		case Qt::TextColorRole:
			break;
        case Qt::BackgroundColorRole:
            if( play_index == row && main_window->is_playing() == true )
                var     =   QColor( 0, 0, 255, 30 );
            else if( status_vec[row].is_new == true )
                var     =   QColor( 255, 0, 0, 30 );
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

	QFileInfo	info	=	file_vec[row];
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
    file_vec.resize( list.size() );
    status_vec.resize( list.size() );

    auto itr2 = file_vec.begin();
    for( auto itr = list.begin(); itr != list.end(); ++itr )
    {
        *itr2    =   *itr;
        ++itr2;
    }
    //file_list   =   std::move(list);

    if( list_from_file.isEmpty() == false )
        compare_with_file();

	refresh_list();
}




/*******************************************************************************
AllModel::clear_from_file_data()
********************************************************************************/
void    AllModel::clear_from_file_data()
{
    list_from_file.clear();
    vec_from_file.clear();
}




/*******************************************************************************
AllModel::compare_with_file()
********************************************************************************/
void    AllModel::compare_with_file()
{
    int     i, j;
    bool    find        =   false;
    bool    find_new    =   false;

    for( i = 0; i < file_vec.size(); i++ )
    {
        find    =   false;
        for( j = 0; j < list_from_file.size(); j++ )
        {
            if( file_vec[i].absoluteFilePath() == list_from_file[j] )
            {
                find    =   true;
                break;
            }
        }

        if( find == false )
        {
            find_new    =   true;
            status_vec[i].is_new    =   true;
        }
        else
        {
            status_vec[i].is_favorite   =   vec_from_file[j] == 1 ? true : false;
            // 刪除資料 加快搜尋速度
            list_from_file.removeAt(j);
            vec_from_file.removeAt(j);
        }
    }

    // sort new
    int     insert_index    =   0;
    if( find_new == true )
    {
        for( i = 0; i < file_vec.size(); i++ )
        {
            if( status_vec[i].is_new == true )
            {
                std::swap( status_vec[i], status_vec[insert_index] );
                std::swap( file_vec[i],   file_vec[insert_index]  );
                insert_index++;
            }
        }
    }

    qDebug() << "compare finish.";
}






/*******************************************************************************
AllModel::clear_played_state()
********************************************************************************/
void    AllModel::clear_played_state()
{
    for( auto &itr : status_vec )
        itr.is_played   =   false;
}





/*******************************************************************************
AllModel::get_random_index()
********************************************************************************/
int    AllModel::get_random_index( bool is_favorite )
{
    std::random_device rd;
    std::uniform_int_distribution<int> dist( 0, file_vec.size()-1 );

    int     idx     =   dist(rd);
    int     size    =   file_vec.size();

    if( is_favorite == false )
    {
        while( status_vec[idx].is_played == true )
            idx     =   (idx+1) % size;
    }
    else
    {
        while( status_vec[idx].is_played == true || status_vec[idx].is_favorite == false )
            idx     =   (idx+1) % size;
    }    

    status_vec[idx].is_played   =   true;
    return  idx;
}



/*******************************************************************************
AllModel::is_status_all_played()
********************************************************************************/
bool    AllModel::is_status_all_played( bool is_favorite )
{
    for( auto &itr : status_vec )
    {
        if( is_favorite == false )
        {
            if( itr.is_played == false )
                return  false;
        }
        else
        {
            if( itr.is_played == false && itr.is_favorite == true )
                return  false;
        }
    }
    return  true;
}




/*******************************************************************************
AllModel::play_by_path()
********************************************************************************/
bool    AllModel::play_by_path( QString path )
{
    if( file_vec.empty() == true )
        return false;

    int     i   =   0;
    for( i = 0; i < file_vec.size(); i++ )
    {
        if( file_vec[i].absoluteFilePath() == path )
            break;        
    }

    if( i >= file_vec.size() )
    {
        MYLOG( LOG::L_WARN, "not found." );
        return  false;
    }

    play_index  =   i;

    //
	QFileInfo	info	=	file_vec[play_index];

    if( main_window->is_playing() == false )
    {
        main_window->set_finish_behavior( FinishBehavior::NONE );
        emit play_signal(info.absoluteFilePath());
        refresh_current();
    }
    else
    {
        main_window->stop_for_user_click();
        main_window->set_finish_behavior( FinishBehavior::USER );
    }
}




/*******************************************************************************
AllModel::play()
********************************************************************************/
bool    AllModel::play( bool is_random, bool is_favorite )
{
    if( file_vec.empty() == true )
        return false;

    if( play_index >= file_vec.size() )
        play_index =   0;

    if( is_random == true )
    {
        clear_played_state();
        play_index  =   get_random_index( is_favorite );
    }

    assert( play_index >= 0 );

	QFileInfo	info	=	file_vec[play_index];
    emit play_signal(info.absoluteFilePath());
    refresh_current();
    return  true;
}




/*******************************************************************************
AllModel::play()
********************************************************************************/
bool    AllModel::play_random( bool is_favorite )
{
    if( file_vec.empty() == true )
        return false;

    play_index  =   get_random_index( is_favorite );    

    assert( play_index >= 0 );

	QFileInfo	info	=	file_vec[play_index];
    emit play_signal(info.absoluteFilePath());
    refresh_current();
    return  true;
}





/*******************************************************************************
AllModel::play()
********************************************************************************/
bool    AllModel::play_next( bool is_repeat )
{
    if( file_vec.empty() == true )
        return false;

    play_index++;
    if( is_repeat == true && play_index == file_vec.size() )
        play_index  =   0;

    if( play_index >= file_vec.size() )
        return  false;

    assert( play_index >= 0 );

	QFileInfo	info	=	file_vec[play_index];
    emit play_signal(info.absoluteFilePath());
    refresh_current();
    return  true;
}





/*******************************************************************************
AllModel::previous()
********************************************************************************/
bool    AllModel::previous()
{
    if( file_vec.empty() == true )
        return false;

    play_index =   play_index > 0 ? play_index-1 : 0;
    assert( play_index >= 0 );

    const QFileInfo&	info	=	file_vec[play_index];
    emit play_signal(info.absoluteFilePath());
    refresh_current();
    return  true;
}




/*******************************************************************************
AllModel::next()
********************************************************************************/
bool    AllModel::next( bool repeat_flag )
{
    if( file_vec.empty() == true )
        return false;

    if( repeat_flag == true )
        play_index  =   (play_index + 1) % file_vec.size();
    else
        play_index  =   play_index < file_vec.size()-1 ? play_index+1 : file_vec.size()-1;

    assert( play_index >= 0 );

    const QFileInfo&	info	=	file_vec[play_index];
    emit play_signal(info.absoluteFilePath());
    refresh_current();
    return  true;
}





/*******************************************************************************
AllModel::set_mainwindow()
********************************************************************************/
void    AllModel::set_mainwindow( MainWindow *mw )
{
    main_window     =   mw;
}





/*******************************************************************************
AllModel::get_file_list()
********************************************************************************/
const QVector<QFileInfo>&    AllModel::get_file_vec()
{
    return  file_vec;
}




/*******************************************************************************
AllModel::get_status_vec()
********************************************************************************/
const QVector<PlayStatus>&   AllModel::get_status_vec()
{
    return  status_vec;
}






/*******************************************************************************
AllModel::set_from_file()
********************************************************************************/
void    AllModel::set_from_file( QStringList&& list, QList<int>&& vec )
{
    list_from_file  =   std::move(list);
    vec_from_file   =   std::move(vec);
}



/*******************************************************************************
AllModel::is_now_play_by_path()
********************************************************************************/
bool    AllModel::is_now_play_by_path( QString path )
{
    QString     current_path    =   file_vec[play_index].absoluteFilePath();
    return  current_path == path;
}





/*******************************************************************************
AllModel::get_status_vec()
********************************************************************************/
QVector<PlayStatus>   AllModel::get_status_vec( const QFileInfoList& list )
{
    if( list.isEmpty() == true )
        return  QVector<PlayStatus>();

    QVector<PlayStatus>     vec;
    vec.resize(list.size());

    int     index       =   0;
    int     push_index  =   0;
    for( auto itr = list.begin(); itr != list.end(); ++itr )
    {
        index   =   file_vec.indexOf( *itr );
        if( index == -1 )
        {
            MYLOG( LOG::L_WARN, "file not found." );
            continue;
        }

        vec[push_index] =   status_vec[index];
        push_index++;        
    }

    return  vec;
}




/*******************************************************************************
AllModel::get_current_play_file()
********************************************************************************/
const QFileInfo&    AllModel::get_current_play_file()
{
    if( file_vec.isEmpty() == true )
        return  QFileInfo();

    return  file_vec[play_index];
}
