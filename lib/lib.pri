#-------------------------------------------------
#
# Project created by QtCreator 2014-02-15T17:29:08
#
#-------------------------------------------------

QT       -= core gui
CONFIG -= qt

TARGET = kiroku
TEMPLATE = lib

DEFINES += KIROKU_LIBRARY

SOURCES += ../lib/intercept.cpp \
    ../lib/sharedmemory.cpp

HEADERS += ../lib/intercept.h \
    ../lib/sharedmemory.h

LIBS += -lrt -dl

QMAKE_CXXFLAGS += -std=gnu++0x
