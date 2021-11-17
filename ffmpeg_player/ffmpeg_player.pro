QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimediawidgets

CONFIG += c++11

CONFIG += console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
	worker.cpp

HEADERS += \
    mainwindow.h \
	worker.h

FORMS += \
    mainwindow.ui
	
INCLUDEPATH   +=  ../src
	
Debug:LIBS    +=  -L../vs2019/x64/Debug -lffmpeg_kernel
Release:LIBS += -L../vs2019/x64/Release -lffmpeg_kernel

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
