#ifndef FILE_MODEL_H
#define FILE_MODEL_H


#include <QAbstractTableModel>
#include <QString>
#include <QDir>
#include <QColor>

#include "all_model.h"





namespace boost{
	class	thread;
} // end namespace boost



class MainWindow;
class AllModel;




class FileModel : public QAbstractTableModel
{
    Q_OBJECT

public:

	FileModel( QObject *parent = 0 );
	~FileModel();

    void    set_root_path( QString path );
    void    init_file_list();
	void	refresh_view();
	void	refresh_row( int row );
    void    refresh_current();

	void	get_file_list();
	int		get_header_count();
    void    set_mainwindow( MainWindow *mw );
    void    set_allmodel( AllModel* am );
    void    update_status_vec( const QFileInfoList& list );

	int		rowCount( const QModelIndex &parent = QModelIndex() ) const ;
	int		columnCount( const QModelIndex &parent = QModelIndex() ) const ;

	QVariant	data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
	QVariant	headerData( int section, Qt::Orientation orientation, int role ) const override;
    
	QVariant	text_data( const QModelIndex &index, int role ) const;
	QVariant	icon_data( const QModelIndex &index, int role ) const;
	QVariant	get_font_color( const QModelIndex &index, int role ) const;

public slots:
	void	double_clicked_slot( const QModelIndex &index );
	void	path_change_slot( const QString &new_path );
	void	refresh_slot();
    void    update_vec_slot();

signals:
	void	refresh_signal();
	void	path_change_signal(QString);

private:
    MainWindow      *main_window    =   nullptr;

    int     file_start_index; // 取得目錄的時候, 檔案起始位置

    QStringList     head_list;
    QString         root_path;
    
    QDir            dir;
    QFileInfoList   file_list;
	QModelIndex		last_index;

    AllModel    *all_model  =   nullptr;

    QVector<PlayStatus>     status_vec;
};


#endif