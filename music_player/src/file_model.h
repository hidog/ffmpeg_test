#ifndef FILE_MODEL_H
#define FILE_MODEL_H


#include <QAbstractTableModel>
#include <QString>
#include <QDir>
#include <QColor>





namespace boost{
	class	thread;
} // end namespace boost





class FileModel : public QAbstractTableModel
{
    Q_OBJECT

public:

	FileModel( QObject *parent = 0 );
	~FileModel();

    void    set_root_path( QString path );
    void    init_file_list();
	void	refresh_view();
	void	refresh_singal( int row );
	void	get_file_list();
	int		get_header_count();

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

signals:
	void	refresh_signal();
	void	path_change_signal(QString);
    void    play_signal(QString);

private:

    QStringList     head_list;
    QString         root_path;
    
    QDir            dir;
    QFileInfoList   file_list;
	QModelIndex		last_index;

};


#endif