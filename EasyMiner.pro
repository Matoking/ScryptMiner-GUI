#-------------------------------------------------
#
# Project created by QtCreator 2011-10-14T18:13:25
#
#-------------------------------------------------

QT       += core gui network

TARGET = ScryptMiner GUI
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    poolparse.cpp \
    json.cpp

HEADERS  += mainwindow.h \
    poolparse.h \
    json.h

FORMS    += mainwindow.ui

RESOURCES += \
    icon.qrc


