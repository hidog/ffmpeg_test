#ifndef FILEWIDGET_H
#define FILEWIDGET_H

#include <QWidget>

#include <vector>




namespace Ui {
	class FileWidget;
} // end namespace Ui

class	FileModel;




class FileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileWidget(QWidget *parent = 0);
    ~FileWidget();

	void	set_root_path( QString path );
    FileModel*  get_model();

public slots:
	void	header_resize_slot( int index, int old_size, int new_size );
	void	path_change_slot( QString path );
    void	refresh_view_slot();

signals:
    
private:

	std::vector<int>	header_width_vec;

	void	set_connect();

    Ui::FileWidget *ui;

	FileModel	*model;
	QString		root_path;
};

#endif // FILEWIDGET_H
