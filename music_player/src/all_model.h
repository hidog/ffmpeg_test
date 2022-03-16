#ifndef ALL_MODEL_H
#define ALL_MODEL_H


#include <QAbstractTableModel>
#include <QString>
#include <QDir>
#include <QColor>

#include <vector>


struct PlayStatus
{
    bool    is_played;
    bool    is_favorite;
    bool    is_new;
    PlayStatus() : is_played{false}, is_new{false}, is_favorite{false} {}
    PlayStatus( bool ft ) : is_played{false}, is_new{false}, is_favorite{ft} {}
};



class MainWindow;




class AllModel : public QAbstractTableModel
{
    Q_OBJECT

public:

	AllModel( QObject *parent = 0 );
	~AllModel();

    void    set_file_list( QFileInfoList&& list );
    void    set_from_file( QStringList&& list, QList<int>&& file );
    void    compare_with_file();    
    void    clear_from_file_data();

	void	refresh_list();
    void    refresh_current();
    void    refresh_index( int col, int row );
	int		get_header_count();
    bool    is_now_play_by_path( QString path );

	int		rowCount( const QModelIndex &parent = QModelIndex() ) const ;
	int		columnCount( const QModelIndex &parent = QModelIndex() ) const ;

    bool    play_by_path( QString path );
    bool    play( bool is_random, bool is_favorite );
    bool    play_random( bool is_favorite );
    bool    previous();
    bool    next( bool repeat_flag );
    bool    play_next( bool is_repeat );
    bool    play_user();
    bool    is_status_all_played( bool is_favorite );

    void    clear_played_state();
    int     get_random_index( bool is_favorite );
    void    set_mainwindow( MainWindow *mw );

	QVariant	data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
	QVariant	headerData( int section, Qt::Orientation orientation, int role ) const override;
    
	QVariant	text_data( const QModelIndex &index, int role ) const;
	QVariant	icon_data( const QModelIndex &index, int role ) const;
	QVariant	get_font_color( const QModelIndex &index, int role ) const;

    const QVector<QFileInfo>&    get_file_vec();
    const QVector<PlayStatus>&   get_status_vec();

public slots:
	void	double_clicked_slot( const QModelIndex &index );
    void	clicked_slot( const QModelIndex &index );
	void	refresh_slot();


signals:
	void	refresh_signal();
	void	path_change_signal(QString);
    void    play_signal(QString);
    void    show_row_signal( int row );

private:

    MainWindow      *main_window    =   nullptr;
    QStringList     head_list;

    QDir    dir;
    int     play_index     =   0;

    QVector<QFileInfo>      file_vec;
    QVector<PlayStatus>     status_vec;

    QStringList     list_from_file;
    QList<int>      vec_from_file;

};


#endif