#include "mainwindow_simple.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SimpleMainWindow w;
    w.show();
    return a.exec();
}