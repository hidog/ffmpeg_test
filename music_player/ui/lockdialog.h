#ifndef LOCKDIALOG_H
#define LOCKDIALOG_H

#include <QDialog>

namespace Ui {
class lockdialog;
}

class lockdialog : public QDialog
{
    Q_OBJECT

public:
    explicit lockdialog(QWidget *parent = nullptr);
    ~lockdialog();

private:
    Ui::lockdialog *ui;
};

#endif // LOCKDIALOG_H
