QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = test_rtp_rtsp

# Input
INCLUDEPATH += ${CV5_SDK_ROOT}linux_x86_64/debug/include
HEADERS += clientwidget.h mymainwindow.h testclient.h
SOURCES += clientwidget.cpp main.cpp mymainwindow.cpp testclient.cpp

LIBS += -lswift -L${HOME}/bqvision/clearvision/linux_x86_64/debug/lib

