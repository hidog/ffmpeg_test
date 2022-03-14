#ifndef LOCKDIALOG_H
#define LOCKDIALOG_H

#include <QDialog>



namespace Ui {
class lockdialog;
} // end namespace Ui






class LockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LockDialog(QWidget *parent = nullptr);
    ~LockDialog();

    void    set_task_name( QString name );


public slots:

    void    progress_slot( int value );
    void    message_slot( QString msg );


private:
    Ui::lockdialog *ui;
};




#endif // LOCKDIALOG_H
