#include <QApplication>
#include <iostream>
#include <string>
#include <QDBusConnection>

#include "deepinwatermark.h"
#include "watermarkadaptor.h"

using namespace std;
//sudo apt install libqt5x11extras5-dev libx11-dev libxext-dev

#define Service "com.deepin.watermark"
#define Path "/com/deepin/watermark"
#define Interface "com.deepin.watermark"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DeepinWatermark watermark;

    WatermarkAdaptor adapter(&watermark);
    Q_UNUSED(adapter);

    if (!QDBusConnection::sessionBus().registerService(Service)) {
        return -1;
    }
    if (!QDBusConnection::sessionBus().registerObject(Path, Interface, &watermark)) {
        return -2;
    }
    return app.exec();
}
