#include "mainwindow.h"

#include <QApplication>



/*
https://www.twblogs.net/a/5c7bc58abd9eee339918aeb3  // rtsp �Ѧ�
https://iter01.com/526101.html  // qt audio �Ѧ�
https://www.zybuluo.com/ltlovezh/note/1498037  timestamp
https://cloud.tencent.com/developer/article/1373966  timestamp

*/


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
