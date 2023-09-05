// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wmadaptor.h"
#include "deepinwmfaker.h"

#include <QGuiApplication>

#define Service "com.deepin.wm"
#define Path "/com/deepin/wm"
#define Interface "com.deepin.wm"

//在wayland下，kwindowsystem默认使用wayland平台的接口，但目前wayland的接口currentDesktop()未实现，所以设置环境变量转为使用xcb平台的接口
__attribute__((constructor))void init()
{
    qputenv("QT_QPA_PLATFORM", "xcb");
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("DeepinWMFaker");

    DeepinWMFaker facker;
    WmAdaptor adapter(&facker);
    Q_UNUSED(adapter)

    if (!QDBusConnection::sessionBus().registerService(Service)) {
        return -1;
    }
    if (!QDBusConnection::sessionBus().registerObject(Path, Interface, &facker)) {
        return -2;
    }

    return app.exec();
}
