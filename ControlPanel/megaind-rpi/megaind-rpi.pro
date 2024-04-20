TEMPLATE = lib
TARGET = megaind-rpi
INCLUDEPATH += ../common

QMAKE_CC = $$QMAKE_CXX
QMAKE_CFLAGS  = $$QMAKE_CXXFLAGS

CONFIG += c++11

CONFIG += staticlib

HEADERS += comm.h \
    megaind.h \
    analog.h \
    dout.h \
    rs485.h

SOURCES += analog.c \
    comm.c \
    megaind.c \
    rs485.c \
    dout.c
