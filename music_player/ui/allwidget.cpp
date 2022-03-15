#include "allwidget.h"
#include "ui_allwidget.h"

#include "../src/all_model.h"

#include <QDebug>






/*******************************************************************************
AllWidget::AllWidget()
********************************************************************************/
AllWidget::AllWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AllWidget)
{
	int		i;

    ui->setupUi(this);

	model	=	new AllModel( this );
	ui->allTView->setModel( model );

	header_width_vec.resize( model->get_header_count() );

	//header_width_vec[0]	=	;	don't set icon width.
	//ui->fileTView->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Fixed );  need set after refresh.
    header_width_vec[0] =   20;
    header_width_vec[1] =   20;
    header_width_vec[2] =   20;
    header_width_vec[3] =   20;
	header_width_vec[4]	=	300;
	header_width_vec[5]	=	70;
	header_width_vec[6]	=	60;
	header_width_vec[7] =	60;
    header_width_vec[8] =   60;
    header_width_vec[9] =   60;
    header_width_vec[10] =   500;

	set_connect();
}






/*******************************************************************************
AllWidget::header_resize_slot()
********************************************************************************/
void	AllWidget::header_resize_slot( int index, int old_size, int new_size )
{
	assert( index < header_width_vec.size() );
	assert( index != 0 );

	header_width_vec[index]		=	new_size;
}




/*******************************************************************************
AllWidget::show_row_slot()
********************************************************************************/
void    AllWidget::show_row_slot( int row )
{
    ui->allTView->selectRow(row);
}





/*******************************************************************************
AllWidget::set_connect()
********************************************************************************/
void	AllWidget::set_connect()
{
	connect(	model,				&AllModel::refresh_signal,			this,		&AllWidget::refresh_list_slot	);
	connect(	model,				&AllModel::show_row_signal,			this,		&AllWidget::show_row_slot	);
    connect(	ui->allTView,		&QTableView::doubleClicked,			model,		&AllModel::double_clicked_slot	);

	connect(	ui->allTView->horizontalHeader(),		&QHeaderView::sectionResized,		this,		&AllWidget::header_resize_slot		);
}










/*******************************************************************************
AllWidget::refresh_view_slot()
********************************************************************************/
void	AllWidget::refresh_list_slot()
{
	int		i;

	ui->allTView->setModel( NULL );
	ui->allTView->setModel( model );

	ui->allTView->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Fixed );

	for( i = 1; i < header_width_vec.size(); i++ )
		ui->allTView->setColumnWidth( i, header_width_vec[i] );
}






/*******************************************************************************
AllWidget::~AllWidget()
********************************************************************************/
AllWidget::~AllWidget()
{
    delete	model;		
    delete	ui;			
}






/*******************************************************************************
AllWidget::set_root_path()
********************************************************************************/
AllModel*  AllWidget::get_model()
{
    return  model;    
}





/*******************************************************************************
AllWidget::QFileInfoList()
********************************************************************************/
void    AllWidget::set_file_list( QFileInfoList&& list )
{
    model->set_file_list( std::move(list) );
}
