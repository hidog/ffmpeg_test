#include "filewidget.h"
#include "ui_filewidget.h"

#include "../src/file_model.h"

#include <QDebug>






/*******************************************************************************
FileWidget::FileWidget()
********************************************************************************/
FileWidget::FileWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileWidget)
{
	int		i;

    ui->setupUi(this);

	model	=	new FileModel( this );
	ui->fileTView->setModel( model );

	header_width_vec.resize( model->get_header_count() );

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
FileWidget::header_resize_slot()
********************************************************************************/
void	FileWidget::header_resize_slot( int index, int old_size, int new_size )
{
	assert( index < header_width_vec.size() );
	assert( index != 0 );

	header_width_vec[index]		=	new_size;
}







/*******************************************************************************
FileWidget::set_connect()
********************************************************************************/
void	FileWidget::set_connect()
{
	connect(	model,				SIGNAL(refresh_signal()),							this,		SLOT(refresh_view_slot())						);
	connect(	model,				SIGNAL(path_change_signal(QString)),				this,		SLOT(path_change_slot(QString))					);
    connect(	ui->fileTView,		SIGNAL(doubleClicked(const QModelIndex&)),			model,		SLOT(double_clicked_slot(const QModelIndex&))	);

	connect(	ui->fileTView->horizontalHeader(),		SIGNAL(sectionResized(int,int,int)),		this,		SLOT(header_resize_slot(int,int,int))		);
}






/*******************************************************************************
FileWidget::refresh_view_slot()
********************************************************************************/
void	FileWidget::refresh_view_slot()
{
	int		i;

	ui->fileTView->setModel( NULL );
	ui->fileTView->setModel( model );

	ui->fileTView->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Fixed );

	for( i = 1; i < header_width_vec.size(); i++ )
		ui->fileTView->setColumnWidth( i, header_width_vec[i] );

}








/*******************************************************************************
FileWidget::path_change_slot()
********************************************************************************/
void	FileWidget::path_change_slot( QString path )
{
	QString		str;
	int			index	=	0;

	// need disconnect. otherwise, model will get more signals.
	disconnect(		ui->pathCBox,		SIGNAL(currentIndexChanged(const QString&)),		model,		SLOT(path_change_slot(const QString&))			);

	//
	ui->pathCBox->clear();
	while(true)
	{
		index	=	path.indexOf( "/", index );
		if( index < 0 )
			break;

		str		=	path.mid( 0, index+1 );
		index++;

		ui->pathCBox->addItem( str );
	}

	// set to last index.
	int		last_index	=	ui->pathCBox->count() - 1;
	ui->pathCBox->setCurrentIndex( last_index );

	//
	connect(		ui->pathCBox,		SIGNAL(currentIndexChanged(const QString&)),		model,		SLOT(path_change_slot(const QString&))			);
}





/*******************************************************************************
FileWidget::~FileWidget()
********************************************************************************/
FileWidget::~FileWidget()
{
    delete	model;		
    delete	ui;			
}





/*******************************************************************************
FileWidget::set_root_path()
********************************************************************************/
void	FileWidget::set_root_path( QString path )
{
	root_path	=	path;
    
    model->set_root_path( root_path );
    model->init_file_list();
}





/*******************************************************************************
FileWidget::set_root_path()
********************************************************************************/
FileModel*  FileWidget::get_model()
{
    return  model;    
}
