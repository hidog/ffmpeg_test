#ifndef ALL_MODEL_H
#define ALL_MODEL_H


#include <QAbstractTableModel>
#include <QString>
#include <QDir>
#include <QColor>





namespace boost{
	class	thread;
} // end namespace boost



class MainWindow;




class AllModel : public QAbstractTableModel
{
    Q_OBJECT

public:

	AllModel( QObject *parent = 0 );
	~AllModel();

    void    set_file_list( QFileInfoList&& list );

	void	refresh_list();
    void    refresh_current();
	int		get_header_count();

	int		rowCount( const QModelIndex &parent = QModelIndex() ) const ;
	int		columnCount( const QModelIndex &parent = QModelIndex() ) const ;

    bool    play();
    bool    previous();
    bool    next();

    void    set_mainwindow( MainWindow *mw );

	QVariant	data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
	QVariant	headerData( int section, Qt::Orientation orientation, int role ) const override;
    
	QVariant	text_data( const QModelIndex &index, int role ) const;
	QVariant	icon_data( const QModelIndex &index, int role ) const;
	QVariant	get_font_color( const QModelIndex &index, int role ) const;

public slots:
	void	double_clicked_slot( const QModelIndex &index );
	void	refresh_slot();

signals:
	void	refresh_signal();
	void	path_change_signal(QString);
    void    play_signal(QString);
    void    show_row_signal( int row );

private:

    QStringList     head_list;
    
    QDir            dir;
    QFileInfoList   file_list;

    int     play_index     =   0;

    MainWindow  *main_window    =   nullptr;
};


#endif