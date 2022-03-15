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
    PlayStatus() : is_played{false}, is_favorite{false} {}
};



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

    bool    play( bool is_random, bool is_favorite );
    bool    play_random( bool is_favorite );
    bool    previous();
    bool    next( bool repeat_flag );
    bool    play_next( bool is_repeat );
    bool    play_user();
    bool    is_status_all_played();

    void    clear_played_state();
    int     get_random_index( bool is_favorite );
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

    std::vector<PlayStatus> status_vec;
};


#endif