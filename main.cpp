#include "mainwindow.h"
#include <QApplication>
#include <iostream>
#include <QDebug>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w((argc == 1) ? nullptr : argv[1]);
    w.show();

    return a.exec();
}
