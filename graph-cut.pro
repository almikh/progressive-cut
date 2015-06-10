#-------------------------------------------------
#
# Project created by QtCreator 2015-02-19T12:13:01
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = graph-cut
TEMPLATE = app


SOURCES +=
        main.cpp\
        mainwindow.cpp\
        graph.cpp

HEADERS +=
        mainwindow.h\
        graph.h

FORMS += mainwindow.ui

QMAKE_CXXFLAGS += -std=c++11

LIBS += `pkg-config opencv --libs`
