QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimediawidgets

CONFIG += c++11

#  CONFIG += console
#   CONFIG -= app_bundle


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    main.cpp \
    ui/mainwindow.cpp \
	src/worker.cpp \
	src/audio_worker.cpp \
	src/video_worker.cpp \
	src/videowidget.cpp \
	src/myslider.cpp


HEADERS += \
    ui/mainwindow.h \
	src/worker.h \
	src/audio_worker.h \
	src/video_worker.h \
	src/videowidget.h \
	src/myslider.h



FORMS += \
    ui/mainwindow.ui



	
INCLUDEPATH   +=  ../src ui src



	
Debug:LIBS    +=  -L../ -lffmpeg_kernelD
Release:LIBS  +=  -L../ -lffmpeg_kernel





# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
