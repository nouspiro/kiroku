#-------------------------------------------------
#
# Project created by QtCreator 2014-02-15T17:28:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += link_pkgconfig
PKGCONFIG += gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0

TARGET = kiroku
TEMPLATE = app

SOURCES += main.cpp\
    mainwindow.cpp \
    ../lib/sharedmemory.cpp \
    recorder.cpp \
    recorderpipeline.cpp \
    recorderbin.cpp \
    busthread.cpp

HEADERS  += mainwindow.h \
    ../lib/sharedmemory.h \
    recorder.h \
    recorderpipeline.h \
    recorderbin.h \
    busthread.h

QMAKE_CXXFLAGS += -std=gnu++0x

FORMS    += mainwindow.ui
