#ifndef ALLWIDGET_H
#define ALLWIDGET_H

#include <QWidget>
#include <QFileInfo>

#include <vector>



namespace Ui {
	class AllWidget;
} // end namespace Ui



class	AllModel;




class AllWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AllWidget(QWidget *parent = 0);
    ~AllWidget();

    void    set_file_list( QFileInfoList&& list );
    AllModel*  get_model();


public slots:
	void	header_resize_slot( int index, int old_size, int new_size );
    void	refresh_list_slot();
    void    show_row_slot( int row );

signals:    

private:

	std::vector<int>	header_width_vec;

	void	set_connect();

    Ui::AllWidget *ui   =   nullptr;
	AllModel	*model  =   nullptr;
};

#endif // FILEWIDGET_H
