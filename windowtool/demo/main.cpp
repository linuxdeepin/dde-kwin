#include <QApplication>
#include <QDebug>
#include <QMap>

#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("windowtool-Demo");
    MainWindow w;
    w.show();

    return app.exec();
}