#-------------------------------------------------
#
# Project created by QtCreator 2014-02-15T17:29:08
#
#-------------------------------------------------

QT       -= core gui

TARGET = kiroku
TEMPLATE = lib

DEFINES += KIROKU_LIBRARY

SOURCES += intercept.cpp \
    sharedmemory.cpp

HEADERS += intercept.h \
    sharedmemory.h

LIBS += -lrt -dl

LIBS += -m32

QMAKE_CXXFLAGS += -std=gnu++0x
QMAKE_CXXFLAGS += -m32

unix {
    target.path = /usr/lib
    INSTALLS += target
}
