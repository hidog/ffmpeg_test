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



https://www.programmersought.com/article/85494009767/
https://programmerclick.com/article/5677107472/

�쥻�Q��QVideoFrame, �S���\, ���QImage�����.



https://www.twblogs.net/a/5ef2639e26bc8c4a8eb3c99e
https://github.com/wang-bin/QtAV/blob/master/src/subtitle/SubtitleProcessorFFmpeg.cpp
https://cpp.hotexamples.com/examples/-/-/hb_subsource_name/cpp-hb_subsource_name-function-examples.html

subtitles filter
�ϥ�libass�wø�ssubtitle�]�r���^�C

*/


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
