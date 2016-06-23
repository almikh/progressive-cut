QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = graph-cut
TEMPLATE = app

SOURCES += \
        main.cpp\
        mainwindow.cpp\
        graph.cpp \
		viewport.cpp

HEADERS += \
        mainwindow.h\
        graph.h \
		matrix.h \
		viewport.h

FORMS += mainwindow.ui

CONFIG += c++11
