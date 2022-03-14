QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

CONFIG += console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
	src/task_manager.cpp \
	src/file_model.cpp \
	src/music_worker.cpp \
	src/play_worker.cpp \
    ui/lockdialog.cpp \
	ui/mainwindow.cpp \
	ui/filewidget.cpp \
    main.cpp


HEADERS += \
	src/task_manager.h \
	src/file_model.h \
	src/music_worker.h \
	src/play_worker \
    ui/lockdialog.h \
    ui/mainwindow.h \
	ui/filewidget.h

FORMS += \
    ui/lockdialog.ui \
    ui/mainwindow.ui \
	ui/filewidget.ui

Debug:LIBS    +=  -L../ -lffmpeg_kernelD
Release:LIBS  +=  -L../ -lffmpeg_kernel

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
