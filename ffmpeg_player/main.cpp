#include "mainwindow.h"

#include <QApplication>



/*
https://www.itread01.com/content/1546396406.html
https://www.twblogs.net/a/5c7bc58abd9eee339918aeb3
https://github.com/cedrus/qt/blob/master/qtmultimedia/src/multimedia/doc/snippets/multimedia-snippets/video.cpp


https://iter01.com/526101.html

https://www.zybuluo.com/ltlovezh/note/1498037  timestamp


https://cloud.tencent.com/developer/article/1373966
https://www.jianshu.com/p/27279255f67e


�쥻�Q��QVideoFrame, �S���\, ���QImage�����.

*/


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
